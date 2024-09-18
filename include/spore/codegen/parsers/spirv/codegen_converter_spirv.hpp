#pragma once

#include <memory>

#include "fmt/format.h"
#include "nlohmann/json.hpp"

#include "spore/codegen/parsers/spirv/ast/spirv_module.hpp"
#include "spore/codegen/parsers/codegen_converter.hpp"
#include "spore/codegen/misc/make_unique_id.hpp"

namespace spore::codegen
{
    void to_json(nlohmann::json& json, const spirv_module& value)
    {
        json["id"] = make_unique_id<spirv_module>();
    }

    struct codegen_converter_spirv : codegen_converter<spirv_module>
    {
        bool convert_ast(const spirv_module& module, nlohmann::json& json) const override
        {
            json = module;
            return true;
        }
    };
}