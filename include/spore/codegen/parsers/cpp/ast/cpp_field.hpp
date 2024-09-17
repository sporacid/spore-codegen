#pragma once

#include <optional>
#include <string>

#include "spore/codegen/parsers/cpp/ast/cpp_attribute.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_flags.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_ref.hpp"

namespace spore::codegen
{
    struct cpp_field : cpp_has_attributes<cpp_field>,
                       cpp_has_flags<cpp_field>
    {
        std::string name;
        std::optional<std::string> default_value;
        cpp_ref type;
    };
}