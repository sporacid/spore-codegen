#pragma once

#include <optional>
#include <string>

#include "spore/codegen/ast/ast_attribute.hpp"
#include "spore/codegen/ast/ast_flags.hpp"
#include "spore/codegen/ast/ast_ref.hpp"

namespace spore::codegen
{
    struct ast_field : ast_has_attributes<ast_field>,
                       ast_has_flags<ast_field>
    {
        std::string name;
        std::optional<std::string> default_value;
        ast_ref type;
    };
}