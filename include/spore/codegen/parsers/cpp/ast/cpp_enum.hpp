#pragma once

#include <string>
#include <vector>

#include "spore/codegen/parsers/cpp/ast/cpp_attribute.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_name.hpp"

namespace spore::codegen
{
    enum class cpp_enum_type
    {
        none,
        enum_,
        enum_class,
    };

    struct cpp_enum_value : cpp_has_attributes<cpp_enum_value>
    {
        std::string name;
        std::int64_t value = 0;
    };

    struct cpp_enum : cpp_has_attributes<cpp_enum>,
                      cpp_has_name<cpp_enum>
    {
        cpp_enum_type type = cpp_enum_type::none;
        cpp_ref base;
        std::vector<cpp_enum_value> values;
        bool nested = false;
        bool definition = false;
    };
}