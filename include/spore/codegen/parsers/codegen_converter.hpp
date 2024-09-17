#pragma once

#include "nlohmann/json.hpp"

namespace spore::codegen
{
    template <typename ast_t>
    struct codegen_converter
    {
        virtual ~codegen_converter() = default;
        [[nodiscard]] virtual bool convert_ast(const ast_t& ast, nlohmann::json& json) const = 0;
    };
}