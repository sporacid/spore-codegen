#pragma once

#define ATTRIBUTE(...) [[clang::annotate(#__VA_ARGS__)]]

namespace spore::codegen::examples::enums
{
    enum class ATTRIBUTE(enum) my_enum
    {
        value1 ATTRIBUTE(name = "CustomName1"),
        value2 ATTRIBUTE(name = "CustomName2"),
        value3 ATTRIBUTE(ignore),
        value4,
    };

    enum class ATTRIBUTE(enum = false) my_ignored_enum
    {
        value1,
        value2,
        value3,
    };
}

#ifndef SPORE_CODEGEN
#include "enum_types.enum.hpp"
#endif