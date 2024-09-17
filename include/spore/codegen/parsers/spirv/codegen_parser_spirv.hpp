#pragma once

#include "spore/codegen/parsers/spirv/ast/spirv_module.hpp"
#include "spore/codegen/parsers/codegen_parser.hpp"

namespace spore::codegen
{
    struct codegen_parser_spirv : codegen_parser<spirv_module>
    {
        template <typename args_t>
        explicit codegen_parser_spirv(const args_t& additional_args)
        {
        }

        bool parse_asts(const std::vector<std::string>& paths, std::vector<spirv_module>& spirv_modules) override
        {
            spirv_modules.clear();
            spirv_modules.reserve(paths.size());
            return true;
        }
    };
}