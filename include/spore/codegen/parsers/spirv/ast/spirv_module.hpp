#pragma once

#include <string>
#include <vector>

#include "spore/codegen/parsers/cpp/ast/cpp_class.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_enum.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_function.hpp"

namespace spore::codegen
{
    struct spirv_module
    {
        std::string path;
        std::vector<std::uint8_t> byte_code;

        bool operator==(const spirv_module& other) const
        {
            return path == other.path;
        }
    };
}