#pragma once

#include <string>
#include <vector>

#include "spore/codegen/ast/ast_attribute.hpp"
#include "spore/codegen/ast/ast_constructor.hpp"
#include "spore/codegen/ast/ast_field.hpp"
#include "spore/codegen/ast/ast_function.hpp"
#include "spore/codegen/ast/ast_name.hpp"

namespace spore::codegen
{
    enum class ast_class_type
    {
        unknown,
        class_,
        struct_,
    };

    struct ast_class : ast_has_attributes<ast_class>,
                       ast_has_name<ast_class>,
                       ast_has_template_params<ast_class>
    {
        ast_class_type type = ast_class_type::unknown;
        std::vector<ast_ref> bases;
        std::vector<ast_field> fields;
        std::vector<ast_function> functions;
        std::vector<ast_constructor> constructors;
    };
}