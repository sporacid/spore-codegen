#pragma once

#include <memory>
#include <vector>

#include "spore/codegen/renderers/codegen_renderer.hpp"

namespace spore::codegen
{
    struct codegen_renderer_composite : codegen_renderer
    {
        std::vector<std::shared_ptr<codegen_renderer>> renderers;

        template <typename... codegen_renderers_t>
        explicit codegen_renderer_composite(codegen_renderers_t&&... renderers)
            : renderers {std::forward<codegen_renderers_t>(renderers)...}
        {
        }

        bool render_file(const std::string& file, const nlohmann::json& data, std::string& result) override
        {
            const auto predicate = [&](const std::shared_ptr<codegen_renderer>& renderer) {
                return renderer->can_render_file(file);
            };

            const auto it_renderer = std::ranges::find_if(renderers, predicate);
            if (it_renderer != renderers.end())
            {
                const std::shared_ptr<codegen_renderer>& renderer = *it_renderer;
                return renderer->render_file(file, data, result);
            }

            return false;
        }

        bool can_render_file(const std::string& file) const override
        {
            const auto predicate = [&](const std::shared_ptr<codegen_renderer>& renderer) {
                return renderer->can_render_file(file);
            };

            return std::ranges::any_of(renderers, predicate);
        }
    };
}