#pragma once

#include <string>
#include <string_view>
#include <variant>

namespace spore::codegen
{
    struct lex_token_base
    {
        std::variant<std::string, std::string_view> variant;

        std::string_view value() const
        {
            if (std::holds_alternative<std::string>(variant))
            {
                return std::get<std::string>(variant);
            }

            return std::get<std::string_view>(variant);
        }
    };

    // clang-format off
    struct lex_keyword : lex_token_base {};
    struct lex_punctuation : lex_token_base {};
    struct lex_scope_begin : lex_token_base {};
    struct lex_scope_end : lex_token_base {};
    struct lex_identifier : lex_token_base {};
    struct lex_type : lex_token_base {};
    struct lex_literal : lex_token_base {};
    struct lex_expression : lex_token_base {};
    // clang-format on

    using lex_token =
        std::variant<
            lex_keyword,
            lex_punctuation,
            lex_scope_begin,
            lex_scope_end,
            lex_identifier,
            lex_type,
            lex_literal,
            lex_expression>;
}