#pragma once

namespace spore::codegen
{
    enum class slang_stage
    {
        none,
        unknown,
        vertex,
        hull,
        domain,
        geometry,
        fragment,
        compute,
        ray_generation,
        ray_intersection,
        ray_any_hit,
        ray_closest_hit,
        ray_miss,
        callable,
        mesh,
        mesh_amplification,
        mesh_dispatch,
#if 0
        none = 1 << 0,
        vertex = 1 << 1,
        hull = 1 << 2,
        domain = 1 << 3,
        geometry = 1 << 4,
        fragment = 1 << 5,
        compute = 1 << 6,
        ray_generation = 1 << 7,
        ray_intersection = 1 << 8,
        ray_any_hit = 1 << 9,
        ray_closest_hit = 1 << 10,
        ray_miss = 1 << 11,
        callable = 1 << 12,
        mesh = 1 << 13,
        mesh_amplification = 1 << 14,
        mesh_dispatch = 1 << 15,
#endif
    };

#if 0
    constexpr slang_stage operator~(slang_stage flags)
    {
        return static_cast<slang_stage>(~static_cast<std::underlying_type_t<slang_stage>>(flags));
    }

    constexpr slang_stage operator|(slang_stage flags, slang_stage other)
    {
        return static_cast<slang_stage>(static_cast<std::underlying_type_t<slang_stage>>(flags) | static_cast<std::underlying_type_t<slang_stage>>(other));
    }

    constexpr slang_stage operator&(slang_stage flags, slang_stage other)
    {
        return static_cast<slang_stage>(static_cast<std::underlying_type_t<slang_stage>>(flags) & static_cast<std::underlying_type_t<slang_stage>>(other));
    }
#endif
}