#pragma once

#include <map>
#include <string>
#include <vector>

#include "spore/codegen/ast/ast_name.hpp"

namespace spore::codegen::ast
{
    namespace detail
    {

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

        //        if (!key.empty())
        //        {
        //            add_pair(std::move(key), std::move(value));
        //        }

        //        constexpr char comma = ',';
        //        constexpr char equal = '=';
        //        constexpr char open_parens = '(';
        //        constexpr char close_parens = ')';
        //        constexpr char any_parens[] {open_parens, close_parens};
        //        constexpr char key_delimiters[] {comma, open_parens};
        //        constexpr std::string_view default_value = "true";
        //
        //        // constexpr std::string_view comma = ",";
        //        // constexpr std::string_view equal = "=";
        //        // constexpr std::string_view parens_open = "(";
        //        // constexpr std::string_view parens_close = ")";
        //        // constexpr std::string_view default_value = "true";
        //        // constexpr std::string_view key_delimiters = "(,";
        //
        //        // a, b = 1, c = (d = 42, e = "wtf", f = (g = 0)), h = "lol"
        //
        //        while (!string.empty())
        //        {
        //            std::size_t index_begin = 0;
        //            std::size_t index_end = string.find_first_of(key_delimiters);
        //
        //            if (index_end != std::string_view::npos && string.at(index_end) == open_parens)
        //            {
        //                std::size_t depth = 1;
        //
        //                while (!string.empty() && depth > 0)
        //                {
        //                }
        //            }
        //        }
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