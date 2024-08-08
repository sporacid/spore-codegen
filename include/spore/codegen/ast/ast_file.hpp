#pragma once

#include <string>
#include <vector>

#include "spore/codegen/ast/ast_class.hpp"
#include "spore/codegen/ast/ast_enum.hpp"
#include "spore/codegen/ast/ast_function.hpp"

namespace spore::codegen
{
    struct ast_file
    {
        std::string path;
        std::string source;
        std::vector<ast_class> classes;
        std::vector<ast_enum> enums;
        std::vector<ast_function> functions;

        bool operator==(const ast_file& other) const
        {
            return path == other.path;
        }
    };
}