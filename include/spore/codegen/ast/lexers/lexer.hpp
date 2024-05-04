#pragma once

#include <array>
#include <string_view>
#include <unordered_set>

#include "spore/codegen/ast/lexers/lex_tokens.hpp"

namespace spore::codegen
{
    namespace detail
    {
        constexpr std::string_view whitespaces = " \t\r\n";

        const inline std::unordered_set<std::string_view> keywords {
            "void",
            "struct",
            "class",
            "union",
            "public",
            "protected",
            "private",
            "namespace",
            "inline",
            "static",
            "const",
            "constexpr",
            "consteval",
            "constinit",
            "auto",
            "decltype",
            "typename",
            "template",
            "concept",
        };

        const inline std::unordered_set<std::string_view> punctuations {
            ":",
            ";",
            ",",
        };

        const inline std::unordered_set<std::string_view> scopes_begin {
            "{",
            "(",
            "[",
            "<",
        };

        const inline std::unordered_set<std::string_view> scopes_end {
            "}",
            ")",
            "]",
            ">",
        };

        void trim_chars(std::string_view& value, std::string_view chars)
        {
            std::size_t index = value.find_first_not_of(chars);
            value.remove_prefix(std::min(index, value.size()));
        }

        void rtrim_chars(std::string_view& value, std::string_view chars)
        {
            std::size_t index = value.find_last_not_of(chars);
            value = value.substr(0, std::max(index + 1, std::size_t {0}));
        }
    }

    struct lexer
    {
        void lex(std::string_view source, std::vector<lex_token>& tokens)
        {
            while (!source.empty())
            {
            }
        }

        template <typename func_t>
        void lex(std::string_view source, func_t&& func)
        {
            while (!source.empty())
            {
            }
        }
    };
}