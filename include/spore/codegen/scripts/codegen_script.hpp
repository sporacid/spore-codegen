#pragma once

#include <string>
#include <functional>

#include "nlohmann/json.hpp"

namespace spore::codegen
{
    #if 0
    struct codegen_script_function
    {
        using func_type = std::function<bool(const std::vector<nlohmann::json>&)>;
        std::string name;
        func_type func;
    };
    #endif

    using codegen_script_function = std::function<bool(const std::vector<nlohmann::json>& args)>;

    struct codegen_script
    {
        std::map<std::string, codegen_script_function> function_map;

        bool invoke(const std::string& name, const std::vector<nlohmann::json>& args)
        {
            const auto it = function_map.find(name);
            return it != function_map.end() ? it->second(args) : false;
        }
    };
}