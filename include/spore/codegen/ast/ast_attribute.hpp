#pragma once

#include <map>
#include <string>
#include <vector>

#include "spore/codegen/ast/ast_name.hpp"

namespace spore::codegen
{
    struct ast_attribute : ast_has_name<ast_attribute>
    {
        std::map<std::string, std::string> values;
    };

    template <typename ast_value_t>
    struct ast_has_attributes
    {
        std::vector<ast_attribute> attributes;
    };
}