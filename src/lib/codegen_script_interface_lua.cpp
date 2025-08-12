#include "spore/codegen/renderers/scripting/codegen_script_interface_lua.hpp"

#include <filesystem>
#include <memory>

#include "sol/sol.hpp"
#include "spdlog/spdlog.h"

// #include "spore/codegen/codegen_macros.hpp"

// SPORE_CODEGEN_PUSH_DISABLE_WARNINGS
// #include "sol/sol.hpp"
// SPORE_CODEGEN_POP_DISABLE_WARNINGS

namespace spore::codegen
{
    namespace detail
    {
        sol::object to_lua(sol::state& lua, const nlohmann::json& json)
        {
            switch (json.type())
            {
                case nlohmann::json::value_t::object: {
                    sol::table lua_table = lua.create_table();

                    for (const auto& [key, value] : json.items())
                    {
                        lua_table[key] = to_lua(lua, value);
                    }

                    return sol::make_object(lua, lua_table);
                }

                case nlohmann::json::value_t::array: {
                    sol::table lua_table = lua.create_table();

                    for (std::size_t index = 0; index < json.size(); ++index)
                    {
                        lua_table[index + 1] = to_lua(lua, json[index]);
                    }

                    return sol::make_object(lua, lua_table);
                }

                case nlohmann::json::value_t::string:
                    return sol::make_object(lua, json.get<std::string>());
                case nlohmann::json::value_t::number_integer:
                    return sol::make_object(lua, json.get<std::int64_t>());
                case nlohmann::json::value_t::number_unsigned:
                    return sol::make_object(lua, json.get<std::uint64_t>());
                case nlohmann::json::value_t::number_float:
                    return sol::make_object(lua, json.get<std::double_t>());
                case nlohmann::json::value_t::boolean:
                    return sol::make_object(lua, json.get<bool>());
                case nlohmann::json::value_t::null:
                default:
                    return sol::make_object(lua, sol::nil);
            }
        }

        nlohmann::json to_json(const sol::object& lua_object)
        {
            if (lua_object.is<sol::lua_nil_t>())
            {
                return nullptr;
            }
            else if (lua_object.is<bool>())
            {
                return lua_object.as<bool>();
            }
            else if (lua_object.is<std::int64_t>())
            {
                return lua_object.as<std::int64_t>();
            }
            else if (lua_object.is<std::uint64_t>())
            {
                return lua_object.as<std::uint64_t>();
            }
            else if (lua_object.is<std::double_t>())
            {
                return lua_object.as<std::double_t>();
            }
            else if (lua_object.is<std::string>())
            {
                return lua_object.as<std::string>();
            }
            else if (lua_object.get_type() == sol::type::table)
            {
                const sol::table& lua_table = lua_object;

                nlohmann::json json;

                bool is_array = true;
                std::size_t max_index = 0;

                for (const auto& [key, value] : lua_table)
                {
                    if (key.is<std::size_t>())
                    {
                        std::size_t index = key.as<std::size_t>();

                        if (index > 0)
                        {
                            if (index > max_index)
                            {
                                max_index = index;
                            }
                        }
                        else
                        {
                            is_array = false;
                            break;
                        }
                    }
                    else
                    {
                        is_array = false;
                        break;
                    }
                }

                if (is_array)
                {
                    json = nlohmann::json::array();

                    for (std::size_t index = 1; index <= max_index; ++index)
                    {
                        json.emplace_back(to_json(lua_table[index]));
                    }
                }
                else
                {
                    json = nlohmann::json::object();

                    for (const auto& [key, value] : lua_table)
                    {
                        if (key.is<std::string>())
                        {
                            const std::string& json_key = key.as<std::string>();
                            json[json_key] = to_json(value);
                        }
                    }
                }

                return json;
            }

            return nlohmann::json();
        }

        template <typename object_t>
        object_t find_object(const sol::object& parent, const std::string_view object_name)
        {
            sol::table object = parent;

            std::size_t index_start = 0;
            std::size_t index_end = object_name.find('.', index_start);

            while (index_end != std::string::npos && object.valid())
            {
                const std::string_view sub_object_name = object_name.substr(index_start, index_end - index_start);
                object = object.get<sol::table>(sub_object_name);

                index_start = index_end + 1;
                index_end = object_name.find('.', index_start);
            }

            if (object.valid())
            {
                const std::string_view sub_object_name = object_name.substr(index_start);
                return object.get<object_t>(sub_object_name);
            }

            return object_t();
        }

        template <typename object_t, typename func_t>
        void for_each_object(const sol::table& table, std::string& prefix, func_t&& func)
        {
            for (const auto& [key, value] : table)
            {
                if (value.is<object_t>())
                {
                    const std::size_t prefix_size = prefix.size();

                    prefix += key.as<std::string>();

                    func(prefix, value.as<object_t>());

                    prefix.resize(prefix_size);
                }

                if (value.is<sol::table>())
                {
                    const std::size_t prefix_size = prefix.size();

                    prefix += key.as<std::string>();
                    prefix += ".";

                    for_each_object<object_t>(value.as<sol::table>(), prefix, func);

                    prefix.resize(prefix_size);
                }
            }
        }

        template <typename object_t, typename func_t>
        void for_each_object(const sol::table& table, func_t&& func)
        {
            std::string prefix;
            for_each_object<object_t>(table, prefix, func);
        }
    }

    struct codegen_script_interface_lua::data
    {
        sol::state lua;
        sol::environment lua_env;

        data()
            : lua_env(lua, sol::create, lua.globals())
        {
        }
    };

    codegen_script_interface_lua::codegen_script_interface_lua(const std::span<const std::string> scripts)
        : _data(std::make_shared<codegen_script_interface_lua::data>())
    {
        sol::state& lua = _data->lua;
        sol::environment& lua_env = _data->lua_env;

        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math, sol::lib::table);

        const auto add_lua_scripts = [&](const std::filesystem::path& script) {
            if (script.extension().string() == ".lua")
            {
                try
                {
                    lua.safe_script_file(script.string(), lua_env);
                }
                catch (const sol::error& e)
                {
                    SPDLOG_WARN("Failed to add LUA script, what={}", e.what());
                }
            }
        };

        for (const std::string& script : scripts)
        {
            const std::filesystem::path script_path {script};

            if (std::filesystem::is_directory(script_path))
            {
                for (const std::filesystem::path& child_script_path : std::filesystem::recursive_directory_iterator(script_path))
                {
                    if (std::filesystem::is_regular_file(child_script_path))
                    {
                        add_lua_scripts(child_script_path);
                    }
                }
            }
            else if (std::filesystem::is_regular_file(script_path))
            {
                add_lua_scripts(script_path);
            }
        }
    }

    bool codegen_script_interface_lua::can_invoke_function(const std::string_view function_name) const
    {
        const sol::environment& lua_env = _data->lua_env;
        sol::function lua_function = detail::find_object<sol::function>(lua_env, function_name);
        return lua_function.valid();
    }

    bool codegen_script_interface_lua::invoke_function(const std::string_view function_name, const std::span<const nlohmann::json> params, nlohmann::json& result)
    {
        const sol::environment& lua_env = _data->lua_env;
        sol::function lua_function = detail::find_object<sol::function>(lua_env, function_name);

        if (not lua_function.valid())
        {
            SPDLOG_ERROR("LUA function not found, name={}", function_name);
            return false;
        }

        std::vector<sol::object> lua_params;
        lua_params.reserve(params.size());

        sol::state& lua = _data->lua;
        std::ranges::transform(params, std::back_inserter(lua_params), std::bind_front(detail::to_lua, std::ref(lua)));

        sol::object lua_result;
        try
        {
            lua_result = lua_function(sol::as_args(lua_params));
        }
        catch (const sol::error& e)
        {
            SPDLOG_ERROR("LUA script error, what={}", e.what());
            return false;
        }

        result = detail::to_json(lua_result);
        return true;
    }

    void codegen_script_interface_lua::for_each_function(const std::function<void(std::string_view)>& func)
    {
        const auto action = [&](const std::string_view function_name, const sol::function& lua_function) {
            func(function_name);
        };

        const sol::environment& lua_env = _data->lua_env;
        detail::for_each_object<sol::function>(lua_env, action);
    }
}