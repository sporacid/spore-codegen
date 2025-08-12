#pragma once

#include <memory>
#include <ranges>

#include "spore/codegen/renderers/scripting/codegen_script_interface.hpp"

namespace spore::codegen
{
    struct codegen_script_interface_lua final : codegen_script_interface
    {
        codegen_script_interface_lua(std::span<const std::string> scripts);

        bool can_invoke_function(const std::string_view function_name) const override;
        bool invoke_function(const std::string_view function_name, const std::span<const nlohmann::json> params, nlohmann::json& result) override;
        void for_each_function(const std::function<void(std::string_view)>& func) override;

      private:
        struct data;
        std::shared_ptr<data> _data;
    };
}