#pragma once

#include <string>
#include <vector>

#include "spore/codegen/parsers/cpp/ast/cpp_attribute.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_constructor.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_field.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_function.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_name.hpp"

namespace spore::codegen
{
    enum class cpp_class_type
    {
        none,
        class_,
        struct_,
    };

    struct cpp_class : cpp_has_attributes<cpp_class>,
                       cpp_has_name<cpp_class>,
                       cpp_has_template_params<cpp_class>
    {
        cpp_class_type type = cpp_class_type::none;
        std::vector<cpp_ref> bases;
        std::vector<cpp_field> fields;
        std::vector<cpp_function> functions;
        std::vector<cpp_constructor> constructors;
        bool nested = false;
        bool definition = false;
    };
}