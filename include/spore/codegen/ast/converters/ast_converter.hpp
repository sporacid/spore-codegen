#pragma once

#include <memory>

#include "nlohmann/json.hpp"

#include "spore/codegen/ast/ast_file.hpp"

namespace spore::codegen
{
    struct ast_converter : std::enable_shared_from_this<ast_converter>
    {
        virtual ~ast_converter() = default;
        virtual bool convert_file(const ast_file& file, nlohmann::json& json) const = 0;
    };
}