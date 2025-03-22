#pragma once

#include <string>

#include "spore/codegen/parsers/cpp/ast/cpp_flags.hpp"

namespace spore::codegen
{
    struct cpp_ref : cpp_has_flags<cpp_ref>
    {
        std::string name;
        bool is_variadic = false;
    };
}