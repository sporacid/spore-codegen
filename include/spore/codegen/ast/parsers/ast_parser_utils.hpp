#pragma once

#include <string>
#include <string_view>

#include "nlohmann/json.hpp"

#include "spore/codegen/utils/strings.hpp"

namespace spore::codegen::ast
{
    inline bool parse_pairs(std::string_view string, nlohmann::json& json)
    {
        constexpr char comma = ',';
        constexpr char equal = '=';
        constexpr char parens_open = '(';
        constexpr char parens_close = ')';
        constexpr char quote = '\'';
        constexpr char double_quote = '"';
        constexpr char escape = '\\';
        constexpr char end = '\0';
        constexpr char parens[] {parens_open, parens_close};
        constexpr char tokens[] {comma, equal, quote, parens_open, parens_close};

        const auto set_token = [&](std::string& token, std::size_t index) {
            token = string.substr(0, index);
            strings::trim(token);
        };

        const auto find_end_quote = [&](char quote, std::size_t index) {
            while (index != std::string_view::npos)
            {
                index = string.find(quote, index + 1);

                const bool is_escaped = index > 0 && string.at(index - 1) == escape;
                const bool is_double_escaped = index > 1 && string.at(index - 2) == escape;

                if (is_escaped && !is_double_escaped)
                {
                    return index;
                }
            }

            return std::string_view::npos;
        };

        const auto find_end_parens = [&](std::size_t index) {
            std::size_t depth = 1;
            while (index != std::string_view::npos && depth > 0)
            {
                index = string.find_first_of(parens, index + 1, std::size(parens));
                if (index != std::string_view::npos)
                {
                    const bool is_open = string.at(index) == parens_open;
                    depth += is_open ? 1 : -1;
                }
            }

            return index;
        };

        const auto add_pair = [&](std::string key, nlohmann::json value) {
            if (key.empty())
            {
                return;
            }

            if (const auto it_json = json.find(key); it_json != json.end())
            {
                nlohmann::json& existing_value = *it_json;
                if (value.is_array() && existing_value.is_array())
                {
                    existing_value.insert(existing_value.end(), value.begin(), value.end());
                }
                else if (value.is_object() && existing_value.is_object())
                {
                    existing_value.merge_patch(value);
                }
                else
                {
                    existing_value = std::move(value);
                }
            }
            else
            {
                json.emplace(std::move(key), std::move(value));
            }
        };

        const auto truncate_string = [&](std::size_t index) {
            if (index != std::string_view::npos)
            {
                string = string.substr(index + 1);
            }
            else
            {
                string = std::string_view {};
            }
        };

        std::string key;

        while (!string.empty())
        {
            std::size_t index = string.find_first_of(tokens, 0, std::size(tokens));
            char token = index != std::string_view::npos ? string.at(index) : end;

            switch (token)
            {
                case quote:
                case double_quote: {
                    index = find_end_quote(token, index);
                    index = string.find_first_of(tokens, index, std::size(tokens));
                    break;
                }

                default: {
                    break;
                }
            }

            switch (token)
            {
                case end:
                case comma: {
                    nlohmann::json json_value;
                    if (key.empty())
                    {
                        set_token(key, index);
                        json_value = true;
                    }
                    else
                    {
                        std::string value;
                        set_token(value, index);
                        json_value = nlohmann::json::parse(value, nullptr, false, false);

                        if (json_value.is_discarded())
                        {
                            // parse as string
                            json_value = nlohmann::json(value);
                        }
                    }

                    add_pair(std::move(key), std::move(json_value));
                    truncate_string(index);
                    break;
                }

                case equal: {
                    set_token(key, index);
                    truncate_string(index);
                    break;
                }

                case parens_open: {
                    std::size_t index_end = find_end_parens(index);
                    if (index_end == std::string_view::npos)
                    {
                        // no matching parenthesis
                        return false;
                    }

                    std::string_view value = string.substr(index + 1, index_end - index - 1);
                    nlohmann::json json_value;

                    if (!parse_pairs(value, json_value))
                    {
                        // invalid inner object
                        return false;
                    }

                    add_pair(std::move(key), std::move(json_value));
                    truncate_string(index_end);
                    break;
                }

                case parens_close: {
                    // invalid closing parenthesis
                    return false;
                }

                default: {
                    break;
                }
            }
        }

        return true;
    }
}