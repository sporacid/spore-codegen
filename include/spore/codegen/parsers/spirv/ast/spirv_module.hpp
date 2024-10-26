#pragma once

#include <string>
#include <vector>

#include "spore/codegen/parsers/spirv/ast/spirv_constant.hpp"
#include "spore/codegen/parsers/spirv/ast/spirv_descriptor.hpp"
#include "spore/codegen/parsers/spirv/ast/spirv_variable.hpp"

namespace spore::codegen
{
    enum class spirv_module_stage
    {
        none,
        vertex,
        fragment,
        compute,
        geometry,
        mesh,
        tesselation_control,
        tesselation_eval,
    };

    struct spirv_module
    {
        std::string path;
        std::string entry_point;
        spirv_module_stage stage = spirv_module_stage::none;
        std::vector<std::uint8_t> byte_code;
        std::vector<spirv_variable> inputs;
        std::vector<spirv_variable> outputs;
        std::vector<spirv_constant> constants;
        std::vector<spirv_descriptor_set> descriptor_sets;

        bool operator==(const spirv_module& other) const
        {
            return path == other.path;
        }
    };
}