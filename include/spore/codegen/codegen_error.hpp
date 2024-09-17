#pragma once

#include <stdexcept>

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
        constexpr codegen_error(const codegen_error_code error_code, const fmt::format_string<args_t...> format, args_t&&... args)
            : std::runtime_error(fmt::format(format, std::forward<args_t>(args)...)),
              error_code(error_code)
        {
        }
    };
}
