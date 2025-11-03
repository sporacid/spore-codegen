#pragma once

#include "spore/codegen/parsers/slang/ast/slang_attribute.hpp"
#include "spore/codegen/parsers/slang/ast/slang_shader_type.hpp"

#include <string>

namespace spore::codegen
{
    struct slang_entry_point
    {
        std::string name;
        slang_shader_type shader_type = slang_shader_type::none;
        std::vector<slang_attribute> attributes;
    };
}