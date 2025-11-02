#pragma once

#include "nlohmann/json.hpp"

#include "spore/codegen/parsers/codegen_converter.hpp"
#include "spore/codegen/parsers/slang/ast/slang_module.hpp"

namespace spore::codegen
{
    inline void to_json(nlohmann::json& json, const slang_module& value)
    {
    }

    struct codegen_converter_slang final : codegen_converter<slang_module>
    {
        bool convert_ast(const slang_module& module, nlohmann::json& json) const override
        {
            json = module;
            return true;
        }
    };
}