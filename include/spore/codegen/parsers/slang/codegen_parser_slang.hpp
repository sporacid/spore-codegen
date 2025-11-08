#pragma once

#include <ranges>

#include "spore/codegen/parsers/codegen_parser.hpp"
#include "spore/codegen/parsers/slang/ast/slang_module.hpp"

namespace spore::codegen
{
    struct codegen_parser_slang final : codegen_parser<slang_module>
    {
        std::vector<std::string> slang_args;

        explicit codegen_parser_slang(const std::span<const std::string> args)
            : slang_args(args.begin(), args.end()) {}

        bool parse_asts(const std::vector<std::string>& paths, std::vector<slang_module>& modules) override;
    };
}