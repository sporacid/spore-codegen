#pragma once

namespace spore::codegen
{
    enum class slang_modifier
    {
        none = 1 << 0,
        shared = 1 << 1,
        no_diff = 1 << 2,
        static_ = 1 << 3,
        const_ = 1 << 4,
        export_ = 1 << 5,
        extern_ = 1 << 6,
        differentiable = 1 << 7,
        mutating = 1 << 8,
        in = 1 << 9,
        out = 1 << 10,
        in_out = 1 << 11,
    };

    constexpr slang_modifier operator~(slang_modifier flags)
    {
        return static_cast<slang_modifier>(~static_cast<std::underlying_type_t<slang_modifier>>(flags));
    }

    constexpr slang_modifier operator|(slang_modifier flags, slang_modifier other)
    {
        return static_cast<slang_modifier>(static_cast<std::underlying_type_t<slang_modifier>>(flags) | static_cast<std::underlying_type_t<slang_modifier>>(other));
    }

    constexpr slang_modifier operator&(slang_modifier flags, slang_modifier other)
    {
        return static_cast<slang_modifier>(static_cast<std::underlying_type_t<slang_modifier>>(flags) & static_cast<std::underlying_type_t<slang_modifier>>(other));
    }
}