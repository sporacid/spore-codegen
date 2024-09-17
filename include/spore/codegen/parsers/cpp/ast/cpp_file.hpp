#pragma once

#include <string>
#include <vector>

#include "spore/codegen/parsers/cpp/ast/cpp_class.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_enum.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_function.hpp"

namespace spore::codegen
{
    struct cpp_file
    {
        std::string path;
        std::string source;
        std::vector<cpp_class> classes;
        std::vector<cpp_enum> enums;
        std::vector<cpp_function> functions;

        bool operator==(const cpp_file& other) const
        {
            return path == other.path;
        }
    };
}