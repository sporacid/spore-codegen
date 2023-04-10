#pragma once

#include <string>
#include <vector>

#include "spore/codegen/ast/ast_attribute.hpp"
#include "spore/codegen/ast/ast_name.hpp"

namespace spore::codegen
{
    struct ast_enum_value : ast_has_attributes<ast_enum_value>
    {
        std::string name;
        std::optional<std::string> value;
    };

    struct ast_enum : ast_has_name<ast_enum>,
                      ast_has_attributes<ast_enum>
    {
        std::vector<ast_enum_value> enum_values;
    };
}