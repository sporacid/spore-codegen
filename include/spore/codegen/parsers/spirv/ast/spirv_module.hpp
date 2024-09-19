#pragma once

#include <string>
#include <vector>

#include "spore/codegen/parsers/spirv/ast/spirv_descriptor.hpp"
#include "spore/codegen/parsers/spirv/ast/spirv_variable.hpp"

namespace spore::codegen
{
    struct spirv_module
    {
        std::string path;
        std::string stage;
        std::vector<std::uint8_t> byte_code;
        std::vector<spirv_variable> inputs;
        std::vector<spirv_variable> outputs;
        std::vector<spirv_descriptor_set> descriptor_sets;

        bool operator==(const spirv_module& other) const
        {
            return path == other.path;
        }
    };
}