#pragma once

namespace spore::codegen::examples::enums
{
    enum class [[enum_]] my_enum
    {
        value1 [[enum_(name = "CustomName1")]],
        value2 [[enum_(name = "CustomName2")]],
        value3 [[enum_(ignore)]],
        value4,
    };

    enum class [[enum_(ignore)]] my_ignored_enum
    {
        value1,
        value2,
        value3,
    };
}

#ifndef SPORE_CODEGEN
#include "enum_types.enum.hpp"
#endif