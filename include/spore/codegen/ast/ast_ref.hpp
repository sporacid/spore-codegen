#pragma once

#include <string>

#include "spore/codegen/ast/ast_flags.hpp"

namespace spore::codegen
{
    struct ast_ref : ast_has_flags<ast_ref>
    {
        std::string name;
    };
}