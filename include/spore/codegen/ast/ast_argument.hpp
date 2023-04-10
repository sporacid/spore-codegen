#pragma once

#include <optional>
#include <string>
#include <vector>

#include "spore/codegen/ast/ast_field.hpp"
#include "spore/codegen/ast/ast_function.hpp"
#include "spore/codegen/ast/ast_name.hpp"
#include "spore/codegen/ast/ast_ref.hpp"

namespace spore::codegen
{
    struct ast_argument : ast_has_attributes<ast_argument>
    {
        std::string name;
        std::optional<std::string> default_value;
        ast_ref type;
    };
}