#pragma once

#include <vector>

#include "spore/codegen/ast/ast_argument.hpp"
#include "spore/codegen/ast/ast_attribute.hpp"
#include "spore/codegen/ast/ast_name.hpp"
#include "spore/codegen/ast/ast_ref.hpp"
#include "spore/codegen/ast/ast_template.hpp"

namespace spore::codegen
{
    struct ast_function : ast_has_name<ast_function>,
                          ast_has_flags<ast_function>,
                          ast_has_attributes<ast_function>,
                          ast_has_template_params<ast_function>
    {
        std::vector<ast_argument> arguments;
        ast_ref return_type;
    };
}