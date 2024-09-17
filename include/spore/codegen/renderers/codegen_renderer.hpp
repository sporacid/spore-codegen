#pragma once

#include <string>

#include "nlohmann/json.hpp"

namespace spore::codegen
{
    struct codegen_renderer
    {
        virtual ~codegen_renderer() = default;
        [[nodiscard]] virtual bool render_file(const std::string& file, const nlohmann::json& data, std::string& result) = 0;
        [[nodiscard]] virtual bool can_render_file(const std::string& file) const = 0;
    };
}