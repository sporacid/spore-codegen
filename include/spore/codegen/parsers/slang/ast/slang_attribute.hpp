#pragma once

#include <string>
#include <variant>
#include <vector>
#include <cmath>

namespace spore::codegen
{
    using slang_attribute_arg = std::variant<std::int32_t, std::float_t, std::string>;

    struct slang_attribute
    {
        std::string name;
        std::vector<slang_attribute_arg> args;
    };
}