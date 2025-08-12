#pragma once

#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "spore/codegen/codegen_macros.hpp"
#include "spore/codegen/renderers/codegen_renderer.hpp"
#include "spore/codegen/renderers/scripting/codegen_script_interface.hpp"

SPORE_CODEGEN_PUSH_DISABLE_WARNINGS
#include "inja/inja.hpp"
SPORE_CODEGEN_POP_DISABLE_WARNINGS

namespace spore::codegen
{
    struct codegen_renderer_inja final : codegen_renderer
    {
        inja::Environment inja_env;
        std::vector<std::string> templates;

        explicit codegen_renderer_inja(const std::shared_ptr<codegen_script_interface>& script_interface, const std::vector<std::string>& templates)
            : templates(templates)
        {
            inja_env.set_trim_blocks(true);
            inja_env.set_lstrip_blocks(true);

            inja_env.add_callback("this", 0,
                [](const inja::Arguments&) -> const nlohmann::json& {
                    static const nlohmann::json default_;
                    return _json_this != nullptr ? *_json_this : default_;
                });

            inja_env.add_callback("include",
                [&](inja::Arguments& args) {
                    const std::string& path = args.at(0)->get<std::string>();
                    const std::span other_args =
                        args.size() > 1
                            ? std::span {args.begin() + 1, args.end()}
                            : std::span<const nlohmann::json*> {};
                    return include_file(path, other_args);
                });

            script_interface->for_each_function([&](const std::string_view function_name) {
                std::string function_name_copy {function_name};
                inja_env.add_callback(function_name_copy, [=](const inja::Arguments& args) -> nlohmann::json {
                    std::vector<nlohmann::json> params;
                    params.reserve(args.size());

                    constexpr auto transformer = [](const nlohmann::json* arg) { return *arg; };
                    std::ranges::transform(args, std::back_inserter(params), transformer);

                    nlohmann::json result;
                    if (script_interface->invoke_function(function_name_copy, params, result))
                    {
                        return result;
                    }

                    return nlohmann::json();
                });
            });
        }

        [[nodiscard]] bool render_file(const std::string& file, const nlohmann::json& data, std::string& result) override
        {
            try
            {
                const auto action = [&] { return inja_env.render_file(file, data); };
                result = with_this(data, action);
                return true;
            }
            catch (const inja::InjaError& err)
            {
                SPDLOG_ERROR("failed to render inja template, file={} error={}", file, err.what());
                return false;
            }
        }

        [[nodiscard]] bool can_render_file(const std::string& file) const override
        {
            return ".inja" == std::filesystem::path(file).extension();
        }

      private:
        static inline thread_local const nlohmann::json* _json_this = nullptr;

        template <typename func_t>
        static auto with_this(const nlohmann::json& json, func_t&& func) -> std::invoke_result_t<std::decay_t<func_t>>
        {
            const nlohmann::json* new_this = std::addressof(json);
            const nlohmann::json* old_this = nullptr;

            defer defer_current_json = [&] { std::swap(_json_this, old_this); };
            std::swap(_json_this, old_this);
            std::swap(_json_this, new_this);

            return func();
        }

        std::string include_file(const std::string& file, std::span<const nlohmann::json*> args)
        {
            nlohmann::json json;

            if (args.size() == 1)
            {
                json = *args[0];
            }
            else if (args.size() > 2)
            {
                for (std::size_t index = 1; index < args.size(); index += 2)
                {
                    json.emplace_back(*args[index]);
                }
            }
            else
            {
                json = nlohmann::json::value_t::null;
            }

            for (const std::string& template_ : templates)
            {
                std::filesystem::path template_abs = std::filesystem::path(template_) / file;

                if (std::filesystem::exists(template_abs))
                {
                    const auto action = [&] { return inja_env.render_file(template_abs.string(), json); };
                    return with_this(json, action);
                }
            }

            throw codegen_error(codegen_error_code::rendering, "cannot find include template, file={}", file);
        }
    };
}