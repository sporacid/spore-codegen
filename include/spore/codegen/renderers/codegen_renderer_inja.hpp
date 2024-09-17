#pragma once

#include <filesystem>
#include <string>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4244)
#include "inja/inja.hpp"
#pragma warning(pop)

#include "base64/base64.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "spore/codegen/renderers/codegen_renderer.hpp"
#include "spore/codegen/utils/json.hpp"
#include "spore/codegen/utils/strings.hpp"

namespace spore::codegen
{
    namespace detail
    {
        inline std::string to_cpp_name(const std::string& value)
        {
            std::string cpp_name = value;

            for (const char character : "<>{}()-+=,.;:/\\ ")
            {
                std::ranges::replace(cpp_name, character, '_');
            }

            return cpp_name;
        }

        inline std::string to_cpp_hex(const std::vector<std::uint8_t>& bytes, const std::size_t width)
        {
            [[maybe_unused]] constexpr char hex_dummy[] = "0xff, ";
            constexpr std::size_t hex_width = sizeof(hex_dummy) - 1;

            std::string hex;
            std::size_t current_width = 0;

            for (const std::uint8_t byte : bytes)
            {
                fmt::format_to(std::back_inserter(hex), "{:#04x}, ", byte);
                current_width += hex_width;

                if (current_width >= width)
                {
                    current_width = 0;
                    hex += "\n";
                }
            }

            return hex;
        }

        inline std::string to_cpp_hex(const nlohmann::json& json, const std::size_t width)
        {
            if (json.is_binary())
            {
                return to_cpp_hex(json.get<std::vector<std::uint8_t>>(), width);
            }

            const std::string& base64 = json.get<std::string>();
            const std::vector<std::uint8_t>& bytes = base64::decode_into<std::vector<std::uint8_t>>(base64.begin(), base64.end());
            return to_cpp_hex(bytes, width);
        }
    }

    struct codegen_renderer_inja final : codegen_renderer
    {
        inja::Environment inja_env;
        std::vector<std::string> templates;

        explicit codegen_renderer_inja(const std::vector<std::string>& templates)
            : templates(templates)
        {
            inja_env.set_trim_blocks(true);
            inja_env.set_lstrip_blocks(true);

            inja_env.add_callback("this", 0,
                [](inja::Arguments& args) -> const nlohmann::json& {
                    static const nlohmann::json default_;
                    return _json_this != nullptr ? *_json_this : default_;
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

            inja_env.add_callback("fs.stem", 1,
                [](inja::Arguments& args) {
                    const std::string& value = args.at(0)->get<std::string>();
                    return std::filesystem::path(value).stem().string();
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

            inja_env.add_callback("cpp.name", 1,
                [](inja::Arguments& args) -> std::string {
                    const std::string& value = args.at(0)->get<std::string>();
                    return detail::to_cpp_name(value);
                });

            inja_env.add_callback("cpp.embed", 1,
                [](inja::Arguments& args) -> std::string {
                    const nlohmann::json* arg0 = args.at(0);
                    return detail::to_cpp_hex(*arg0, 80);
                });

            inja_env.add_callback("cpp.embed", 2,
                [](inja::Arguments& args) -> std::string {
                    const nlohmann::json* arg0 = args.at(0);
                    const std::size_t arg1 = args.at(1)->get<std::size_t>();
                    return detail::to_cpp_hex(*arg0, arg1);
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