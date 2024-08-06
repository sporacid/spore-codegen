#pragma once

#include <type_traits>

namespace spore::codegen
{
    enum class ast_flags
    {
        none = 0,
        const_ = 1 << 0,
        volatile_ = 1 << 1,
        mutable_ = 1 << 2,
        static_ = 1 << 3,
        virtual_ = 1 << 4,
        default_ = 1 << 5,
        delete_ = 1 << 6,
        public_ = 1 << 7,
        private_ = 1 << 8,
        protected_ = 1 << 9,
        constexpr_ = 1 << 10,
        consteval_ = 1 << 11,
        lvalue_ref = 1 << 12,
        rvalue_ref = 1 << 13,
        pointer = 1 << 14,
    };

    constexpr ast_flags operator~(ast_flags flags)
    {
        return static_cast<ast_flags>(~static_cast<std::underlying_type_t<ast_flags>>(flags));
    }

    constexpr ast_flags operator|(ast_flags flags, ast_flags other)
    {
        return static_cast<ast_flags>(static_cast<std::underlying_type_t<ast_flags>>(flags) | static_cast<std::underlying_type_t<ast_flags>>(other));
    }

    constexpr ast_flags operator&(ast_flags flags, ast_flags other)
    {
        return static_cast<ast_flags>(static_cast<std::underlying_type_t<ast_flags>>(flags) & static_cast<std::underlying_type_t<ast_flags>>(other));
    }

    template <typename ast_value_t>
    struct ast_has_flags
    {
        ast_flags flags = ast_flags::none;

        [[nodiscard]] bool has_flags(ast_flags other) const
        {
            return (flags & other) == other;
        }
    };
}