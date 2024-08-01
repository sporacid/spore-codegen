#pragma once

#include <map>
#include <string>
#include <vector>

#include "spore/codegen/ast/ast_name.hpp"

namespace spore::codegen::ast
{
    bool parse_pairs_to_json(std::string_view string, nlohmann::json& json)
    {
        constexpr char comma = ',';
        constexpr char equal = '=';
        constexpr char parens_open = '(';
        constexpr char parens_close = ')';
        constexpr char quote = '\'';
        constexpr char double_quote = '"';
        constexpr char escape = '\\';
        constexpr char end = '\0';
        constexpr char quotes[] {quote, double_quote};
        constexpr char parens[] {parens_open, parens_close};
        constexpr char tokens[] {comma, equal, quote, parens_open, parens_close};

        const auto set_token = [&](std::string& token, std::size_t index) {
            token = string.substr(0, index);
            trim_chars(token, whitespaces);
            // trim_chars(token, std::string_view {quotes, std::size(quotes)});
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

        const auto add_pair_to_json = [&](std::string key, nlohmann::json value) {
            if (key.empty())
            {
                return;
            }

            if (const auto it_json = json.find(key); it_json != json.end())
            {
                nlohmann::json& existing_value = *it_json;
                if (existing_value.is_array())
                {
                    existing_value.emplace_back(std::move(value));
                }
                else if (existing_value.is_object() && value.is_object())
                {
                    existing_value.merge_patch(value);
                }
                else
                {
                    nlohmann::json json_array;
                    json_array.emplace_back(std::move(existing_value));
                    json_array.emplace_back(std::move(value));
                    existing_value = std::move(json_array);
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
                        json_value = nlohmann::json::parse(std::move(value), nullptr, false, false);
                    }

                    if (json_value.is_discarded())
                    {
                        // invalid json
                        return false;
                    }

                    add_pair_to_json(std::move(key), std::move(json_value));
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
                    nlohmann::json json_value = nlohmann::json::object();

                    if (!parse_pairs_to_json(value, json_value))
                    {
                        // invalid inner object
                        return false;
                    }

                    add_pair_to_json(std::move(key), std::move(json_value));
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

    void parse_pairs_v2(std::string_view string, std::map<std::string, std::string>& pairs)
    {
        constexpr char comma = ',';
        constexpr char equal = '=';
        constexpr char open_parens = '(';
        constexpr char close_parens = ')';
        constexpr char space = ' ';
        constexpr char quote = '"';
        constexpr char end = '\0';
        constexpr char tokens[] {comma, equal, open_parens, close_parens};
        constexpr std::string_view default_value = "true";

        const auto set_token = [&](std::string& token, std::size_t index) {
            token = string.substr(0, index);

            trim_chars(token, whitespaces);
            trim_chars(token, std::string_view(&quote, 1));

            if (index != std::string_view::npos)
            {
                string = string.substr(index + 1);
            }
            else
            {
                string = std::string_view {};
            }
        };

        const auto add_pair = [&](std::string&& key, std::string&& value) {
            if (value.empty())
            {
                value = default_value;
            }

            if (const auto it_pair = pairs.find(key); it_pair != pairs.end())
            {
                std::string& pair_value = it_pair->second;
                pair_value += comma;
                pair_value += space;
                pair_value += value;
            }
            else
            {
                pairs.emplace(std::move(key), std::move(value));
            }

            key.clear();
            value.clear();
        };

        std::string key;
        std::string value;
        std::size_t depth = 0;
        std::size_t index = 0;

        while (!string.empty())
        {
            index = string.find_first_of(tokens, index, std::size(tokens));

            char token = index != std::string_view::npos ? string.at(index) : end;
            bool handled = false;

            switch (token)
            {
                case end:
                case comma:
                    if (depth == 0)
                    {
                        if (key.empty())
                        {
                            set_token(key, index);
                            index = 0;
                        }
                        else
                        {
                            set_token(value, index);
                            index = 0;
                        }

                        add_pair(std::move(key), std::move(value));
                        handled = true;
                    }
                    break;

                case equal:
                    if (depth == 0)
                    {
                        set_token(key, index);
                        index = 0;
                        handled = true;
                    }
                    break;

                case open_parens:
                    ++depth;
                    break;

                case close_parens:
                    --depth;
                    break;

                default:
                    break;
            }

            if (!handled)
            {
                ++index;
            }
        }
    }

    void parse_pairs(std::string_view pairs, std::map<std::string, std::string>& attributes)
    {
        constexpr std::string_view key_delimiter = ",";
        constexpr std::string_view value_delimiter = "=";
        constexpr std::string_view default_value = "true";

        while (!pairs.empty())
        {
            std::size_t index_begin = 0;
            std::size_t index_end = pairs.find(key_delimiter);
            std::size_t count = index_end != std::string_view::npos ? index_end - index_begin : std::string_view::npos;

            std::string_view pair = pairs.substr(index_begin, count);
            std::size_t index_value = pair.find(value_delimiter);

            std::string_view key;
            std::string_view value;

            if (index_value != std::string_view::npos)
            {
                key = pair.substr(0, index_value - 1);
                value = pair.substr(index_value + 1);
            }
            else
            {
                key = pair;
                value = default_value;
            }

            trim_chars(key, whitespaces);
            trim_chars(value, whitespaces);

            std::string key_string {key};

            const auto it_attribute = attributes.find(key_string);
            if (it_attribute != attributes.end())
            {
                std::string& value_string = it_attribute->second;
                value_string += key_delimiter;
                value_string += value;
            }
            else
            {
                attributes.emplace(std::move(key_string), std::string(value));
            }

            pairs.remove_prefix(count != std::string_view::npos ? count + 1 : pairs.size());
        }
    }
}