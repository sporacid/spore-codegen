#pragma once

#include <vector>

#include "spore/codegen/parsers/cpp/ast/cpp_argument.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_attribute.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_flags.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_template.hpp"

namespace spore::codegen
{
    struct cpp_constructor : cpp_has_attributes<cpp_constructor>,
                             cpp_has_flags<cpp_constructor>,
                             cpp_has_template_params<cpp_constructor>
    {
        std::vector<cpp_argument> arguments;
    };
}