#pragma once

#include <memory>
#include <string>

#include "nlohmann/json.hpp"

#include "spore/codegen/ast/ast_file.hpp"

namespace spore::codegen
{
    struct codegen_renderer : std::enable_shared_from_this<codegen_renderer>
    {
        virtual ~codegen_renderer() = default;
        virtual bool render_file(const std::string& file, const nlohmann::json& data, std::string& result) = 0;
        virtual bool can_render_file(const std::string& file) const = 0;
    };
}