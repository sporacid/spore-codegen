#pragma once

#include "spdlog/spdlog.h"

#include "spore/codegen/parsers/codegen_parser.hpp"
#include "spore/codegen/parsers/spirv/ast/spirv_module.hpp"

namespace spore::codegen
{
    struct codegen_parser_spirv final : codegen_parser<spirv_module>
    {
        template <typename args_t>
        explicit codegen_parser_spirv(const args_t& args)
        {
            if (std::size(args) > 0)
            {
                SPDLOG_WARN("SPIR-V parser does not support additional arguments, they will be ignored");
            }
        }

        bool parse_asts(const std::vector<std::string>& paths, std::vector<spirv_module>& modules) override;
    };
}