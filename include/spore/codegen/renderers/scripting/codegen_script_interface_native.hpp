#pragma once

#include <functional>
#include <unordered_map>

#include "spdlog/spdlog.h"

#include "spore/codegen/renderers/scripting/codegen_script_interface.hpp"
#include "spore/codegen/renderers/scripting/codegen_script_utils.hpp"

namespace spore::codegen
{
    struct codegen_script_interface_native : codegen_script_interface
    {
        using function_t = std::function<void(std::span<const nlohmann::json>, nlohmann::json&)>;

        struct hash : std::hash<std::string>, std::hash<std::string_view>
        {
            using is_transparent = void;
            using std::hash<std::string>::operator();
            using std::hash<std::string_view>::operator();
        };

        struct equal_to : std::equal_to<std::string>, std::equal_to<std::string_view>
        {
            using is_transparent = void;
            using std::equal_to<std::string>::operator();
            using std::equal_to<std::string_view>::operator();
        };

        std::unordered_map<std::string, function_t, hash, equal_to> function_map;

        bool can_invoke_function(const std::string_view function_name) const override
        {
            return function_map.contains(function_name);
        }

        bool invoke_function(const std::string_view function_name, const std::span<const nlohmann::json> params, nlohmann::json& result) override
        {
            const auto it_function = function_map.find(function_name);
            if (it_function == function_map.end())
            {
                SPDLOG_ERROR("native function not found, name={}", function_name);
                return false;
            }

            const function_t& function = it_function->second;
            function(params, result);
            return true;
        }

        void for_each_function(const std::function<void(std::string_view)>& func) override
        {
            for (const auto& [function_name, function] : function_map)
            {
                func(function_name);
            }
        }

        static codegen_script_interface_native make_default();
    };
}