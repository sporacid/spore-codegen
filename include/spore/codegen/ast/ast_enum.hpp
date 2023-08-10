#pragma once

#include <string>
#include <vector>

#include "spore/codegen/ast/ast_attribute.hpp"
#include "spore/codegen/ast/ast_name.hpp"

namespace spore::codegen
{
    enum class ast_enum_type
    {
        none,
        enum_,
        enum_class,
    };

    struct ast_enum_value : ast_has_attributes<ast_enum_value>
    {
        std::string name;
        std::int64_t value = 0;
    };

    struct ast_enum : ast_has_attributes<ast_enum>,
                      ast_has_name<ast_enum>
    {
        ast_enum_type type = ast_enum_type::none;
        ast_ref base;
        std::vector<ast_enum_value> values;
        bool nested = false;
    };
}