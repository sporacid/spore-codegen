#pragma once

#include <concepts>
#include <memory>
#include <vector>

#include "spore/codegen/renderers/codegen_renderer.hpp"

namespace spore::codegen
{
    struct codegen_renderer_composite final : codegen_renderer
    {
        std::vector<std::unique_ptr<codegen_renderer>> renderers;

        template <std::derived_from<codegen_renderer>... renderers_t>
        explicit codegen_renderer_composite(std::unique_ptr<renderers_t>&&... in_renderers)
        {
            renderers.reserve(sizeof...(renderers_t));
            (renderers.emplace_back(std::move(in_renderers)), ...);
        }

        [[nodiscard]] bool render_file(const std::string& file, const nlohmann::json& data, std::string& result) override
        {
            const auto predicate = [&](const std::unique_ptr<codegen_renderer>& renderer) {
                return renderer->can_render_file(file);
            };

            const auto it_renderer = std::ranges::find_if(renderers, predicate);
            if (it_renderer != renderers.end())
            {
                const std::unique_ptr<codegen_renderer>& renderer = *it_renderer;
                return renderer->render_file(file, data, result);
            }

            return false;
        }

        [[nodiscard]] bool can_render_file(const std::string& file) const override
        {
            const auto predicate = [&](const std::unique_ptr<codegen_renderer>& renderer) {
                return renderer->can_render_file(file);
            };

            return std::ranges::any_of(renderers, predicate);
        }
    };
}