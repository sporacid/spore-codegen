#include "spore/codegen/renderers/scripting/codegen_script_interface_native.hpp"

#include <filesystem>

#include "base64/base64.hpp"

#include "spore/codegen/renderers/scripting/codegen_script_utils.hpp"
#include "spore/codegen/utils/json.hpp"
#include "spore/codegen/utils/strings.hpp"
#include "spore/codegen/utils/yaml.hpp"

namespace spore::codegen
{
    namespace detail
    {
        std::string to_cpp_name(const std::string_view value)
        {
            std::string cpp_name {value};

            for (const char character : "<>{}()-+=,.;:/\\ ")
            {
                std::ranges::replace(cpp_name, character, '_');
            }

            return cpp_name;
        }

        std::string to_cpp_hex(const std::vector<std::uint8_t>& bytes, const std::size_t width)
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

        std::string to_cpp_hex(const nlohmann::json& json, const std::size_t width)
        {
            if (json.is_binary())
            {
                return to_cpp_hex(json.get<std::vector<std::uint8_t>>(), width);
            }

            const std::string& base64 = json.get<std::string>();
            const std::vector<std::uint8_t>& bytes = base64::decode_into<std::vector<std::uint8_t>>(base64.begin(), base64.end());
            return to_cpp_hex(bytes, width);
        }

        void to_flattened(const nlohmann::json& json, const std::string_view separator, nlohmann::json& flattened, std::vector<std::string>& keys)
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

        nlohmann::json to_flattened(const nlohmann::json& json, const std::string_view separator)
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
    }

    codegen_script_interface_native codegen_script_interface_native::make_default()
    {
        codegen_script_interface_native script_interface;

        script_interface.function_map.emplace("truthy", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [json, property] = scripts::get_params<nlohmann::json, std::optional<std::string>>(params);
            result = property.has_value() ? json::truthy(json, property.value()) : json::truthy(json);
        });

        script_interface.function_map.emplace("contains", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [haystack, needle] = scripts::get_params<std::string, std::string>(params);
            result = haystack.find(needle) != std::string::npos;
        });

        script_interface.function_map.emplace("replace", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [value, from, to] = scripts::get_params<std::string, std::string, std::string>(params);
            strings::replace_all(value, from, to);
            result = nlohmann::json();
        });

        script_interface.function_map.emplace("starts_with", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [value, prefix] = scripts::get_params<std::string, std::string>(params);
            result = value.starts_with(prefix);
        });

        script_interface.function_map.emplace("ends_with", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [value, suffix] = scripts::get_params<std::string, std::string>(params);
            result = value.ends_with(suffix);
        });

        script_interface.function_map.emplace("trim_start", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [value, chars] = scripts::get_params<std::string, std::optional<std::string>>(params);
            strings::trim_start(value, chars.has_value() ? chars.value() : strings::whitespaces);
            result = std::move(value);
        });

        script_interface.function_map.emplace("trim_end", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [value, chars] = scripts::get_params<std::string, std::optional<std::string>>(params);
            strings::trim_end(value, chars.has_value() ? chars.value() : strings::whitespaces);
            result = std::move(value);
        });

        script_interface.function_map.emplace("trim", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [value, chars] = scripts::get_params<std::string, std::optional<std::string>>(params);
            strings::trim(value, chars.has_value() ? chars.value() : strings::whitespaces);
            result = std::move(value);
        });

        script_interface.function_map.emplace("flatten", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [json, separator] = scripts::get_params<nlohmann::json, std::optional<std::string>>(params);
            result = detail::to_flattened(json, separator.value_or("."));
        });

        script_interface.function_map.emplace("json", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [json, indent] = scripts::get_params<nlohmann::json, std::optional<std::size_t>>(params);
            result = json.dump(indent.value_or(2));
        });

        script_interface.function_map.emplace("yaml", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [json, indent] = scripts::get_params<nlohmann::json, std::optional<std::size_t>>(params);
            result = yaml::to_yaml(json, indent.value_or(2));
        });

        script_interface.function_map.emplace("fs.absolute", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [path] = scripts::get_params<std::string>(params);
            result = std::filesystem::absolute(std::filesystem::path(std::move(path))).string();
        });

        script_interface.function_map.emplace("fs.extension", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [path] = scripts::get_params<std::string>(params);
            result = std::filesystem::path(std::move(path)).extension().string();
        });

        script_interface.function_map.emplace("fs.stem", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [path] = scripts::get_params<std::string>(params);
            result = std::filesystem::path(std::move(path)).stem().string();
        });

        script_interface.function_map.emplace("fs.filename", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [path] = scripts::get_params<std::string>(params);
            result = std::filesystem::path(std::move(path)).filename().string();
        });

        script_interface.function_map.emplace("fs.directory", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [path] = scripts::get_params<std::string>(params);
            result = std::filesystem::path(std::move(path)).parent_path().string();
        });

        script_interface.function_map.emplace("cpp.name", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [name] = scripts::get_params<std::string>(params);
            result = detail::to_cpp_name(name);
        });

        script_interface.function_map.emplace("cpp.embed", [](const std::span<const nlohmann::json> params, nlohmann::json& result) {
            auto [value, width] = scripts::get_params<std::string, std::optional<std::size_t>>(params);
            result = detail::to_cpp_hex(value, width.value_or(80));
        });

        return script_interface;
    }
}