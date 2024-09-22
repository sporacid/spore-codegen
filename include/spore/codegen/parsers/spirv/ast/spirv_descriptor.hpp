#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "spore/codegen/parsers/spirv/ast/spirv_variable.hpp"

namespace spore::codegen
{
    enum class spirv_descriptor_kind : std::uint8_t
    {
        none,
        uniform,
        sampler,
        storage,
        unknown = std::numeric_limits<std::uint8_t>::max(),
    };

    struct spirv_descriptor_binding
    {
        spirv_descriptor_kind kind = spirv_descriptor_kind::none;
        std::string name;
        std::string type;
        std::size_t index = 0;
        std::size_t count = 0;
        std::vector<spirv_variable> variables;
    };

    struct spirv_descriptor_set
    {
        std::size_t index = 0;
        std::vector<spirv_descriptor_binding> bindings;
    };
}