#pragma once

#include <string>
#include <vector>

#include "spore/codegen/parsers/codegen_parser.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_file.hpp"

namespace spore::codegen
{
    struct codegen_parser_cpp final : codegen_parser<cpp_file>
    {
        std::vector<std::string> additional_args;

        template <typename args_t>
        explicit codegen_parser_cpp(const args_t& additional_args)
            : additional_args(std::begin(additional_args), std::end(additional_args))
        {
        }

        bool parse_asts(const std::vector<std::string>& paths, std::vector<cpp_file>& cpp_files) override;
    };
}
