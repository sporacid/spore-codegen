#pragma once

#include <cstdint>
#include <string>

namespace spore::codegen
{
    enum class spirv_variable_kind
    {
        none,
        input,
        output,
    };

    struct spirv_variable
    {
        spirv_variable_kind kind = spirv_variable_kind::none;
        std::string name;
        std::size_t location = 0;
        std::size_t size = 0;
        std::size_t count = 0;
        std::size_t format = 0;
    };
}