#pragma once

#include <cstdint>
#include <string>
#include <variant>

namespace spore::codegen
{
    enum class spirv_scalar_kind : std::uint8_t
    {
        none,
        bool_,
        int_,
        float_,
    };

    struct spirv_scalar
    {
        spirv_scalar_kind kind = spirv_scalar_kind::none;
        std::size_t width = 0;
        bool signed_ = false;
    };

    struct spirv_vec
    {
        spirv_scalar scalar;
        std::size_t components = 0;
    };

    struct spirv_mat
    {
        spirv_scalar scalar;
        std::size_t rows = 0;
        std::size_t cols = 0;
        bool row_major = false;
    };

    struct spirv_struct
    {
        std::string name;
    };

    struct spirv_image
    {
        std::size_t dims = 0;
    };

    struct spirv_array
    {
        using variant_t = std::variant<spirv_scalar, spirv_vec, spirv_mat, spirv_struct, spirv_image>;
        variant_t type;
        std::array<std::size_t, 8> dims {};
    };

    using spirv_type =
        std::variant<
            spirv_scalar,
            spirv_vec,
            spirv_mat,
            spirv_struct,
            spirv_image,
            spirv_array>;
}