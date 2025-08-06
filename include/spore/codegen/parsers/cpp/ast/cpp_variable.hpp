#pragma once

#include <optional>
#include <string>

#include "spore/codegen/parsers/cpp/ast/cpp_attribute.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_flags.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_name.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_ref.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_template.hpp"

namespace spore::codegen
{
    struct cpp_variable : cpp_has_attributes<cpp_variable>,
                          cpp_has_name<cpp_variable>,
                          cpp_has_flags<cpp_variable>,
                          cpp_has_template_params<cpp_variable>
    {
        cpp_ref type;
        std::optional<std::string> default_value;
    };
}