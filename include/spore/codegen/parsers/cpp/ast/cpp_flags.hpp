#pragma once

#include <type_traits>

namespace spore::codegen
{
    enum class cpp_flags
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
        constinit_ = 1 << 12,
        inline_ = 1 << 13,
        explicit_ = 1 << 14,
        lvalue_ref = 1 << 15,
        rvalue_ref = 1 << 16,
        pointer = 1 << 17,
    };

    constexpr cpp_flags operator~(cpp_flags flags)
    {
        return static_cast<cpp_flags>(~static_cast<std::underlying_type_t<cpp_flags>>(flags));
    }

    constexpr cpp_flags operator|(cpp_flags flags, cpp_flags other)
    {
        return static_cast<cpp_flags>(static_cast<std::underlying_type_t<cpp_flags>>(flags) | static_cast<std::underlying_type_t<cpp_flags>>(other));
    }

    constexpr cpp_flags operator&(cpp_flags flags, cpp_flags other)
    {
        return static_cast<cpp_flags>(static_cast<std::underlying_type_t<cpp_flags>>(flags) & static_cast<std::underlying_type_t<cpp_flags>>(other));
    }

    template <typename cpp_value_t>
    struct cpp_has_flags
    {
        cpp_flags flags = cpp_flags::none;

        [[nodiscard]] bool has_flags(cpp_flags other) const
        {
            return (flags & other) == other;
        }
    };
}