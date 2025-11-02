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

        explicit codegen_parser_cpp(const std::span<const std::string> args)
            : additional_args(args.begin(), args.end())
        {
        }

        bool parse_asts(const std::vector<std::string>& paths, std::vector<cpp_file>& cpp_files) override;
    };
}
