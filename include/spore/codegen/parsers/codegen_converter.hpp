#pragma once

#include <ranges>

#include "nlohmann/json.hpp"

namespace spore::codegen
{
    template <typename ast_t>
    struct codegen_converter
    {
        virtual ~codegen_converter() = default;
        [[nodiscard]] virtual bool convert_ast(const ast_t& ast, nlohmann::json& json) const = 0;
        [[nodiscard]] virtual bool convert_asts(std::span<const ast_t> asts, nlohmann::json& json) const = 0;
    };
}