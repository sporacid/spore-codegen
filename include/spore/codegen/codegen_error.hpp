#pragma once

#include <stdexcept>
#include <string_view>

#include "fmt/format.h"

namespace spore::codegen
{
    enum class codegen_error_code
    {
        none,
        unknown,
        invalid,
        io,
        configuring,
        parsing,
        conversion,
        rendering,
    };

    struct codegen_error final : std::runtime_error
    {
        codegen_error_code error_code;

        template <typename... args_t>
        codegen_error(const codegen_error_code error_code, const std::string_view format, args_t&&... args)
            : std::runtime_error(fmt::vformat(format, fmt::make_format_args(args...))),
              error_code(error_code)
        {
        }
    };
}
