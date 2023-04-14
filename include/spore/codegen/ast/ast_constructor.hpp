#pragma once

#include <vector>

#include "spore/codegen/ast/ast_argument.hpp"
#include "spore/codegen/ast/ast_attribute.hpp"
#include "spore/codegen/ast/ast_flags.hpp"
#include "spore/codegen/ast/ast_template.hpp"

namespace spore::codegen
{
    struct ast_constructor : ast_has_attributes<ast_constructor>,
                             ast_has_flags<ast_constructor>,
                             ast_has_template_params<ast_constructor>
    {
        std::vector<ast_argument> arguments;
    };
}