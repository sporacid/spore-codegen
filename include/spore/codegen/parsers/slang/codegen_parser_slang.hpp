#pragma once

#include <ranges>

#include "spdlog/spdlog.h"

#include "spore/codegen/parsers/codegen_parser.hpp"
#include "spore/codegen/parsers/slang/ast/slang_module.hpp"

namespace spore::codegen
{
    struct codegen_parser_slang final : codegen_parser<slang_module>
    {
        explicit codegen_parser_slang(const std::span<const std::string> args)
        {
            if (not args.empty())
            {
                SPDLOG_WARN("Slang parser does not support additional arguments, they will be ignored");
            }
        }

        bool parse_asts(const std::vector<std::string>& paths, std::vector<slang_module>& modules) override;
    };
}