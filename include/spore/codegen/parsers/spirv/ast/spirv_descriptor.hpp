#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "spore/codegen/parsers/spirv/ast/spirv_variable.hpp"

namespace spore::codegen
{
    enum class spirv_descriptor_kind
    {
        none,
        uniform,
        sampler,
    };

    struct spirv_descriptor_binding
    {
        std::size_t index = 0;
        std::string name;
        spirv_descriptor_kind kind = spirv_descriptor_kind::none;
        std::vector<spirv_variable> variables;
    };

    struct spirv_descriptor_set
    {
        std::size_t index = 0;
        std::vector<spirv_descriptor_binding> bindings;
    };
}