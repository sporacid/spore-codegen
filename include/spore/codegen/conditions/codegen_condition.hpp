#pragma once

namespace spore::codegen
{
    template <typename ast_t>
    struct codegen_condition
    {
        virtual ~codegen_condition() = default;
        [[nodiscard]] virtual bool match_ast(const ast_t& ast) const = 0;
    };
}