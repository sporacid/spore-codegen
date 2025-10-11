#pragma once

#include <string>
#include <vector>

#include "spore/codegen/parsers/cpp/ast/cpp_flags.hpp"

namespace spore::codegen
{
    struct cpp_ref : cpp_has_flags<cpp_ref>
    {
        std::string name;
        std::string base_name;
        std::vector<std::size_t> extent;
        bool is_variadic = false;
    };
}