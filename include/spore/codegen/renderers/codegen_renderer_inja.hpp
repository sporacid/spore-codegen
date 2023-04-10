#pragma once

#include <filesystem>
#include <string>

#pragma warning(push)
#pragma warning(disable : 4244)
#include "inja/inja.hpp"
#pragma warning(pop)

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "spore/codegen/renderers/codegen_renderer.hpp"

namespace spore::codegen
{
    namespace details
    {
        bool is_truthy(const nlohmann::json& json)
        {
            bool truthy = false;
            switch (json.type())
            {
                case nlohmann::json::value_t::string:
                    truthy = !json.get<std::string>().empty();
                    break;
                case nlohmann::json::value_t::array:
                    truthy = !json.get<std::vector<nlohmann::json>>().empty();
                    break;
                case nlohmann::json::value_t::object:
                    truthy = !json.get<std::map<std::string, nlohmann::json>>().empty();
                    break;
                case nlohmann::json::value_t::number_integer:
                    truthy = json.get<std::int64_t>() != 0;
                    break;
                case nlohmann::json::value_t::number_unsigned:
                    truthy = json.get<std::uint64_t>() != 0;
                    break;
                case nlohmann::json::value_t::number_float:
                    truthy = json.get<double>() != 0.;
                    break;
                case nlohmann::json::value_t::boolean:
                    truthy = json.get<bool>();
                    break;
                default:
                    break;
            }
            return truthy;
        }

        bool is_truthy(const nlohmann::json& json, std::string property)
        {
            property.insert(property.begin(), '/');
            std::replace(property.begin(), property.end(), '.', '/');

            nlohmann::json::json_pointer json_pointer(property);
            return json.contains(json_pointer) && is_truthy(json[json_pointer]);
        }

        std::string to_cpp_name(const std::string& value)
        {
            std::string cpp_name = value;

            for (char character : "<>{}()-+=,.;:/\\ ")
            {
                std::replace(cpp_name.begin(), cpp_name.end(), character, '_');
            }

            return cpp_name;
        }
    }

    struct codegen_renderer_inja : codegen_renderer
    {
        inja::Environment inja_env;

        codegen_renderer_inja()
        {
            inja_env.set_trim_blocks(true);
            inja_env.set_lstrip_blocks(true);

            inja_env.add_callback("truthy", 1,
                [](inja::Arguments& args) {
                    const nlohmann::json& json = *args.at(0);
                    return details::is_truthy(json);
                });

            inja_env.add_callback("truthy", 2,
                [](inja::Arguments& args) {
                    const nlohmann::json& json = *args.at(0);
                    std::string property = args.at(1)->get<std::string>();
                    return details::is_truthy(json, std::move(property));
                });

            inja_env.add_callback("cpp_name", 1,
                [](inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    return details::to_cpp_name(value);
                });
        }

        bool render_file(const std::string& file, const nlohmann::json& data, std::string& result) override
        {
            try
            {
                result = inja_env.render_file(file, data);
                return true;
            }
            catch (const inja::InjaError& err)
            {
                SPDLOG_ERROR("failed to render inja template, file={} error={}", file, err.what());
                return false;
            }
        }

        bool can_render_file(const std::string& file) const override
        {
            return ".inja" == std::filesystem::path(file).extension();
        }
    };
}