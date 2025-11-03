#pragma once

namespace spore::codegen
{
    enum class slang_shader_type
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
}