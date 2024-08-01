#pragma once

#include <memory>
#include <string>
#include <vector>

#include "spore/codegen/ast/ast_file.hpp"

namespace spore::codegen
{
    struct ast_parser : std::enable_shared_from_this<ast_parser>
    {
        virtual ~ast_parser() = default;
        virtual bool parse_files(const std::vector<std::string>& paths, std::vector<ast_file>& files) = 0;
    };
}