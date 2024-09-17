#pragma once

#include <vector>

#include "spore/codegen/parsers/cpp/ast/cpp_argument.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_attribute.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_name.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_ref.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_template.hpp"

namespace spore::codegen
{
    struct cpp_function : cpp_has_attributes<cpp_function>,
                          cpp_has_flags<cpp_function>,
                          cpp_has_name<cpp_function>,
                          cpp_has_template_params<cpp_function>
    {
        std::vector<cpp_argument> arguments;
        cpp_ref return_type;

        bool is_variadic() const
        {
            const auto predicate = [](const cpp_argument& arg) { return arg.is_variadic; };
            return std::any_of(arguments.begin(), arguments.end(), predicate);
        }
    };
}