#pragma once

#include <string>
#include <vector>

namespace spore::codegen
{
    template <typename ast_t>
    struct codegen_parser
    {
        virtual ~codegen_parser() = default;
        virtual bool parse_asts(const std::vector<std::string>& paths, std::vector<ast_t>& asts) = 0;
    };
}