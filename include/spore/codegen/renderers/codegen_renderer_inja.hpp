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
#include "spore/codegen/utils/json.hpp"
#include "spore/codegen/utils/strings.hpp"

namespace spore::codegen
{
    namespace detail
    {
        std::string to_cpp_name(const std::string& value)
        {
            std::string cpp_name = value;

            for (char character : "<>{}()-+=,.;:/\\ ")
            {
                std::replace(cpp_name.begin(), cpp_name.end(), character, '_');
            }

            return cpp_name;
        }

        std::string include_template(inja::Environment& env, const inja::Arguments& args, const std::vector<std::string>& template_paths)
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
                    return json::truthy(json);
                });

            inja_env.add_callback("truthy", 2,
                [](inja::Arguments& args) {
                    const nlohmann::json& json = *args.at(0);
                    std::string property = args.at(1)->get<std::string>();
                    return json::truthy(json, std::move(property));
                });

            inja_env.add_callback("contains", 2,
                [](inja::Arguments& args) {
                    const std::string& value = args.at(0)->get<std::string>();
                    const std::string& str = args.at(1)->get<std::string>();
                    return value.find(str) != std::string::npos;
                });

            inja_env.add_callback("replace", 3,
                [](inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    const std::string& from = args.at(1)->get<std::string>();
                    const std::string& to = args.at(2)->get<std::string>();
                    strings::replace_all(value, from, to);
                    return value;
                });

            inja_env.add_callback("starts_with", 2,
                [](inja::Arguments& args) {
                    const std::string& value = args.at(0)->get<std::string>();
                    const std::string& prefix = args.at(1)->get<std::string>();
                    return value.starts_with(prefix);
                });

            inja_env.add_callback("ends_with", 2,
                [](inja::Arguments& args) {
                    const std::string& value = args.at(0)->get<std::string>();
                    const std::string& suffix = args.at(1)->get<std::string>();
                    return value.ends_with(suffix);
                });

            inja_env.add_callback("trim_start", 1,
                [](inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    strings::trim_start(value);
                    return value;
                });

            inja_env.add_callback("trim_start", 2,
                [](inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    const std::string& chars = args.at(1)->get<std::string>();
                    strings::trim_start(value, chars);
                    return value;
                });

            inja_env.add_callback("trim_end", 1,
                [](inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    strings::trim_end(value);
                    return value;
                });

            inja_env.add_callback("trim_end", 2,
                [](inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    const std::string& chars = args.at(1)->get<std::string>();
                    strings::trim_end(value, chars);
                    return value;
                });

            inja_env.add_callback("trim", 1,
                [](inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    strings::trim(value);
                    return value;
                });

            inja_env.add_callback("trim", 2,
                [](inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    const std::string& chars = args.at(1)->get<std::string>();
                    strings::trim(value, chars);
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
                    return json.dump(static_cast<int>(indent));
                });

            inja_env.add_callback("yaml", 1,
                [](inja::Arguments& args) {
                    const nlohmann::json& json = *args.at(0);
                    return yaml::to_yaml(json, 2);
                });

            inja_env.add_callback("yaml", 2,
                [](inja::Arguments& args) {
                    const nlohmann::json& json = *args.at(0);
                    std::size_t indent = *args.at(1);
                    return yaml::to_yaml(json, indent);
                });

            inja_env.add_callback("fs.absolute", 1,
                [](inja::Arguments& args) {
                    const std::string& value = args.at(0)->get<std::string>();
                    return std::filesystem::absolute(std::filesystem::path(value)).string();
                });

            inja_env.add_callback("fs.extension", 1,
                [](inja::Arguments& args) {
                    const std::string& value = args.at(0)->get<std::string>();
                    return std::filesystem::path(value).extension().string();
                });

            inja_env.add_callback("fs.filename", 1,
                [](inja::Arguments& args) {
                    const std::string& value = args.at(0)->get<std::string>();
                    return std::filesystem::path(value).filename().string();
                });

            inja_env.add_callback("fs.directory", 1,
                [](inja::Arguments& args) {
                    const std::string& value = args.at(0)->get<std::string>();
                    return std::filesystem::path(value).parent_path().string();
                });

            inja_env.add_callback("cpp_name", 1,
                [](inja::Arguments& args) -> std::string {
                    const std::string& value = args.at(0)->get<std::string>();
                    return detail::to_cpp_name(value);
                });

            inja_env.add_callback("include",
                [&, template_paths](inja::Arguments& args) {
                    return detail::include_template(inja_env, args, template_paths);
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