#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include "spore/codegen/utils/strings.hpp"

namespace spore::codegen::paths
{
    template <typename arg_t, typename... args_t>
    std::string combine(arg_t&& arg, args_t&&... args)
    {
        std::filesystem::path path {std::forward<arg_t>(arg)};
        (path.append(std::forward<args_t>(args)), ...);

        std::string value = path.string();
        strings::replace_all(value, "\\", "/");

        return value;
    }
}
