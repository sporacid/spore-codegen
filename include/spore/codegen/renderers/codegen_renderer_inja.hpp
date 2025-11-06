#pragma once

#include <filesystem>
#include <format>
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
                std::format_to(std::back_inserter(hex), "{:#04x}, ", byte);
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

        inline void to_flattened(const nlohmann::json& json, const std::string_view separator, nlohmann::json& flattened, std::vector<std::string>& keys)
        {
            if (not json.is_object())
            {
                return;
            }

            for (const auto& [key, json_value] : json.items())
            {
                std::string flat_key = strings::join(separator, keys);

                if (not flat_key.empty())
                {
                    flat_key += separator;
                }

                flat_key += key;

                if (json_value.is_object())
                {
                    keys.push_back(key);

                    flattened[flat_key] = true;

                    to_flattened(json_value, separator, flattened, keys);

                    keys.pop_back();
                }
                else
                {
                    flattened[flat_key] = json_value;
                }
            }
        }

        inline nlohmann::json to_flattened(const nlohmann::json& json, const std::string_view separator)
        {
            if (not json.is_object())
            {
                return json;
            }

            nlohmann::json flattened = nlohmann::json::object();
            std::vector<std::string> keys;

            to_flattened(json, separator, flattened, keys);

            return flattened;
        }

        inline std::string format_impl(const std::string_view format, const std::span<const nlohmann::json* const> args)
        {
            const auto make_format_args = [&]<std::size_t... indices_v>(std::index_sequence<indices_v...>) {
                return std::make_format_args(*args[indices_v]...);
            };

            switch (args.size())
            {
                case 0:
                    return std::string(format);
                case 1:
                    return std::vformat(format, make_format_args(std::make_index_sequence<1>()));
                case 2:
                    return std::vformat(format, make_format_args(std::make_index_sequence<2>()));
                case 3:
                    return std::vformat(format, make_format_args(std::make_index_sequence<3>()));
                case 4:
                    return std::vformat(format, make_format_args(std::make_index_sequence<4>()));
                case 5:
                    return std::vformat(format, make_format_args(std::make_index_sequence<5>()));
                case 6:
                    return std::vformat(format, make_format_args(std::make_index_sequence<6>()));
                case 7:
                    return std::vformat(format, make_format_args(std::make_index_sequence<7>()));
                case 8:
                    return std::vformat(format, make_format_args(std::make_index_sequence<8>()));
                default:
                    throw codegen_error(codegen_error_code::rendering, "unsupported format");
            }
        }

        inline nlohmann::json find_by(const nlohmann::json& json, const std::string_view field_name, const nlohmann::json& field_value)
        {
            if (!json.is_array())
            {
                throw codegen_error(codegen_error_code::rendering, "find_by: first arg is not array");
            }

            for (auto const& object : json)
            {
                const bool is_object_valid = object.is_object() && object.contains(field_name);

                if (is_object_valid && object[field_name] == field_value)
                {
                    return object;
                }
            }

            throw codegen_error(codegen_error_code::rendering, "find_by: object not found");
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
                [](const inja::Arguments&) -> const nlohmann::json& {
                    static const nlohmann::json default_;
                    return _json_this != nullptr ? *_json_this : default_;
                });

            inja_env.add_callback("truthy", 1,
                [](const inja::Arguments& args) {
                    const nlohmann::json& json = *args.at(0);
                    return json::truthy(json);
                });

            inja_env.add_callback("truthy", 2,
                [](const inja::Arguments& args) {
                    const nlohmann::json& json = *args.at(0);
                    std::string property = args.at(1)->get<std::string>();
                    return json::truthy(json, std::move(property));
                });

            inja_env.add_callback("contains", 2,
                [](const inja::Arguments& args) {
                    const std::string& value = args.at(0)->get<std::string>();
                    const std::string& str = args.at(1)->get<std::string>();
                    return value.find(str) != std::string::npos;
                });

            inja_env.add_callback("regex_match", 2,
                [](const inja::Arguments& args) {
                    const std::string& input = args.at(0)->get<std::string>();
                    const std::string& regex = args.at(1)->get<std::string>();

                    return strings::regex_match(input, regex);
                });

            inja_env.add_callback("regex_search", 2,
                [](const inja::Arguments& args) {
                    const std::string& input = args.at(0)->get<std::string>();
                    const std::string& regex = args.at(1)->get<std::string>();
                    nlohmann::json json = nlohmann::json::array();

                    strings::for_each_by_regex(input, regex, [&](const std::string_view match) {
                        json.push_back(match);
                    });

                    return json;
                });

            inja_env.add_callback("find_by", 3,
                [](const inja::Arguments& args) {
                    const nlohmann::json& json = *args.at(0);
                    const std::string& field_name = args.at(1)->get<std::string>();
                    const nlohmann::json& field_value = args.at(2)->get<nlohmann::json>();
                    return detail::find_by(json, field_name, field_value);
                });

            inja_env.add_callback("replace", 3,
                [](const inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    const std::string& from = args.at(1)->get<std::string>();
                    const std::string& to = args.at(2)->get<std::string>();
                    strings::replace_all(value, from, to);
                    return value;
                });

            inja_env.add_callback("format",
                [](const inja::Arguments& args) {
                    const std::string& format = args.at(0)->get<std::string>();
                    const std::span format_args {args.begin() + 1, args.end()};
                    return detail::format_impl(format, format_args);
                });

            inja_env.add_callback("starts_with", 2,
                [](const inja::Arguments& args) {
                    const std::string& value = args.at(0)->get<std::string>();
                    const std::string& prefix = args.at(1)->get<std::string>();
                    return value.starts_with(prefix);
                });

            inja_env.add_callback("ends_with", 2,
                [](const inja::Arguments& args) {
                    const std::string& value = args.at(0)->get<std::string>();
                    const std::string& suffix = args.at(1)->get<std::string>();
                    return value.ends_with(suffix);
                });

            inja_env.add_callback("trim_start", 1,
                [](const inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    strings::trim_start(value);
                    return value;
                });

            inja_env.add_callback("trim_start", 2,
                [](const inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    const std::string& chars = args.at(1)->get<std::string>();
                    strings::trim_start(value, chars);
                    return value;
                });

            inja_env.add_callback("trim_end", 1,
                [](const inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    strings::trim_end(value);
                    return value;
                });

            inja_env.add_callback("trim_end", 2,
                [](const inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    const std::string& chars = args.at(1)->get<std::string>();
                    strings::trim_end(value, chars);
                    return value;
                });

            inja_env.add_callback("trim", 1,
                [](const inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    strings::trim(value);
                    return value;
                });

            inja_env.add_callback("trim", 2,
                [](const inja::Arguments& args) {
                    std::string value = args.at(0)->get<std::string>();
                    const std::string& chars = args.at(1)->get<std::string>();
                    strings::trim(value, chars);
                    return value;
                });

            inja_env.add_callback("split_into_words", 1,
                [](const inja::Arguments& args) {
                    const std::string& input = args.at(0)->get<std::string>();
                    nlohmann::json json = nlohmann::json::array();

                    strings::for_each_words(input, [&](const std::string_view word) {
                        json.push_back(word);
                    });

                    return json;
                });

            inja_env.add_callback("to_camel_case", 1,
                [](const inja::Arguments& args) {
                    const std::string& input = args.at(0)->get<std::string>();
                    return strings::to_camel_case(input);
                });

            inja_env.add_callback("to_upper_camel_case", 1,
                [](const inja::Arguments& args) {
                    const std::string& input = args.at(0)->get<std::string>();
                    return strings::to_upper_camel_case(input);
                });

            inja_env.add_callback("to_snake_case", 1,
                [](const inja::Arguments& args) {
                    const std::string& input = args.at(0)->get<std::string>();
                    return strings::to_snake_case(input);
                });

            inja_env.add_callback("to_upper_snake_case", 1,
                [](const inja::Arguments& args) {
                    const std::string& input = args.at(0)->get<std::string>();
                    return strings::to_upper_snake_case(input);
                });

            inja_env.add_callback("to_title_case", 1,
                [](const inja::Arguments& args) {
                    const std::string& input = args.at(0)->get<std::string>();
                    return strings::to_title_case(input);
                });

            inja_env.add_callback("to_sentence_case", 1,
                [](const inja::Arguments& args) {
                    const std::string& input = args.at(0)->get<std::string>();
                    return strings::to_sentence_case(input);
                });

            inja_env.add_callback("flatten", 1,
                [](const inja::Arguments& args) -> nlohmann::json {
                    const nlohmann::json* arg0 = args.at(0);
                    return detail::to_flattened(*arg0, ".");
                });

            inja_env.add_callback("flatten", 2,
                [](const inja::Arguments& args) -> nlohmann::json {
                    const nlohmann::json* arg0 = args.at(0);
                    const std::string& arg1 = args.at(1)->get<std::string>();
                    return detail::to_flattened(*arg0, arg1);
                });

            inja_env.add_callback("json", 1,
                [](const inja::Arguments& args) {
                    const nlohmann::json& json = *args.at(0);
                    return json.dump(2);
                });

            inja_env.add_callback("json", 2,
                [](const inja::Arguments& args) {
                    const nlohmann::json& json = *args.at(0);
                    const std::size_t indent = *args.at(1);
                    return json.dump(static_cast<int>(indent));
                });

            inja_env.add_callback("yaml", 1,
                [](const inja::Arguments& args) {
                    const nlohmann::json& json = *args.at(0);
                    return yaml::to_yaml(json, 2);
                });

            inja_env.add_callback("yaml", 2,
                [](const inja::Arguments& args) {
                    const nlohmann::json& json = *args.at(0);
                    const std::size_t indent = *args.at(1);
                    return yaml::to_yaml(json, indent);
                });

            inja_env.add_callback("fs.absolute", 1,
                [](const inja::Arguments& args) {
                    const std::string& value = args.at(0)->get<std::string>();
                    return std::filesystem::absolute(std::filesystem::path(value)).string();
                });

            inja_env.add_callback("fs.extension", 1,
                [](const inja::Arguments& args) {
                    const std::string& value = args.at(0)->get<std::string>();
                    return std::filesystem::path(value).extension().string();
                });

            inja_env.add_callback("fs.stem", 1,
                [](const inja::Arguments& args) {
                    const std::string& value = args.at(0)->get<std::string>();
                    return std::filesystem::path(value).stem().string();
                });

            inja_env.add_callback("fs.filename", 1,
                [](const inja::Arguments& args) {
                    const std::string& value = args.at(0)->get<std::string>();
                    return std::filesystem::path(value).filename().string();
                });

            inja_env.add_callback("fs.directory", 1,
                [](const inja::Arguments& args) {
                    const std::string& value = args.at(0)->get<std::string>();
                    return std::filesystem::path(value).parent_path().string();
                });

            inja_env.add_callback("cpp.name", 1,
                [](const inja::Arguments& args) -> std::string {
                    const std::string& value = args.at(0)->get<std::string>();
                    return detail::to_cpp_name(value);
                });

            inja_env.add_callback("cpp.embed", 1,
                [](const inja::Arguments& args) -> std::string {
                    const nlohmann::json* arg0 = args.at(0);
                    return detail::to_cpp_hex(*arg0, 80);
                });

            inja_env.add_callback("cpp.embed", 2,
                [](const inja::Arguments& args) -> std::string {
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

        std::string include_file(const std::string& file, const std::span<const nlohmann::json*> args)
        {
            nlohmann::json json;

            if (args.size() == 1)
            {
                json = *args[0];
            }
            else if (args.size() > 1)
            {
                for (const nlohmann::json* arg : args)
                {
                    json.emplace_back(*arg);
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