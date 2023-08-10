#pragma once

#include <memory>

#include "nlohmann/json.hpp"

#include "spore/codegen/ast/ast_file.hpp"

namespace spore::codegen
{
    struct ast_condition : std::enable_shared_from_this<ast_condition>
    {
        virtual ~ast_condition() = default;
        virtual bool matches_condition(const ast_file& file) const = 0;
    };
}