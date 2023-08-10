#pragma once

#include <filesystem>
#include <string>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4244)
#include "inja/inja.hpp"
#pragma warning(pop)

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "spore/codegen/renderers/codegen_renderer.hpp"
#include "spore/codegen/utils/strings.hpp"

namespace spore::codegen
{
    namespace detail
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

        std::string render_include_template(inja::Environment& env, const inja::Arguments& args, const std::vector<std::string>& template_paths)
        {
            if (args.empty())
            {
                throw codegen_error(codegen_error_code::rendering, "cannot include template, missing file");
            }

            nlohmann::json json_data;
            if (args.size() == 2)
            {
                json_data = *args.at(1);
            }
            else if (args.size() > 2)
            {
                std::vector<nlohmann::json> json_values;
                for (std::size_t index = 1; index < args.size(); index += 2)
                {
                    json_values.emplace_back(*args.at(index));
                }

                json_data = std::move(json_values);
            }

            std::string template_file = args.at(0)->get<std::string>();
            for (const std::string& template_path : template_paths)
            {
                std::filesystem::path template_file_abs = std::filesystem::path(template_path) / template_file;
                if (std::filesystem::exists(template_file_abs))
                {
                    return env.render_file(template_file_abs.string(), json_data);
                }
            }

            throw codegen_error(codegen_error_code::rendering, "cannot find include template, file={}", template_file);
        }
    }

    struct codegen_renderer_inja : codegen_renderer
    {
        inja::Environment inja_env;

        explicit codegen_renderer_inja(const std::vector<std::string>& template_paths)
        {
            inja_env.set_trim_blocks(true);
            inja_env.set_lstrip_blocks(true);

            inja_env.add_callback("this", 0,
                [](inja::Arguments& args) -> nlohmann::json {
                    return _json_this != nullptr ? *_json_this : nlohmann::json();
                });

            inja_env.add_callback("truthy", 1,
                [](inja::Arguments& args) {
                    const nlohmann::json& json = *args.at(0);
                    return detail::is_truthy(json);
                });

            inja_env.add_callback("truthy", 2,
                [](inja::Arguments& args) {
                    const nlohmann::json& json = *args.at(0);
                    std::string property = args.at(1)->get<std::string>();
                    return detail::is_truthy(json, std::move(property));
                });

            inja_env.add_callback("cpp_name", 1,
                [](inja::Arguments& args) -> std::string {
                    std::string value = args.at(0)->get<std::string>();
                    return detail::to_cpp_name(value);
                });

            inja_env.add_callback("contains", 2,
                [](inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    std::string str = args.at(1)->get<std::string>();
                    return value.find(str) != std::string::npos;
                });

            inja_env.add_callback("replace", 3,
                [](inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    std::string from = args.at(1)->get<std::string>();
                    std::string to = args.at(2)->get<std::string>();
                    strings::replace_all(value, from, to);
                    return value;
                });

            inja_env.add_callback("json", 1,
                [](inja::Arguments& args) {
                    const nlohmann::json& json = *args.at(0);
                    return json.dump(2);
                });

            inja_env.add_callback("json", 2,
                [](inja::Arguments& args) {
                    const nlohmann::json& json = *args.at(0);
                    std::size_t indent = *args.at(1);
                    return json.dump(indent);
                });

            inja_env.add_callback("abs_path", 1,
                [](inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    return std::filesystem::absolute(std::filesystem::path(value)).string();
                });

            inja_env.add_callback("include",
                [&, template_paths](inja::Arguments& args) {
                    return detail::render_include_template(inja_env, args, template_paths);
                });
        }

        bool render_file(const std::string& file, const nlohmann::json& data, std::string& result) override
        {
            try
            {
                defer defer_current_json = [&] { _json_this = nullptr; };
                _json_this = &data;
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

      private:
        static inline thread_local const nlohmann::json* _json_this = nullptr;
    };
}