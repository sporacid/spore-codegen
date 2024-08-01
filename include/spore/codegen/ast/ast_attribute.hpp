#pragma once

#include <map>
#include <string>
#include <vector>

#include "spore/codegen/ast/ast_name.hpp"

namespace spore::codegen
{
    struct ast_attribute
    {
        std::variant<bool, std::int64_t, std::double_t, std::map<std::string, std::string>> value;
    };

    template <typename ast_value_t>
    struct ast_has_attributes
    {
        std::map<std::string, std::string> attributes;
    };
}