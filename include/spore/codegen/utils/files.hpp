#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include "picosha2.h"

#include "spore/codegen/utils/yaml.hpp"

namespace spore::codegen::files
{
    namespace detail
    {
        enum class json_file_type
        {
            none,
            json,
            bson,
            yaml,
        };

        inline json_file_type get_json_type(std::string_view path)
        {
            constexpr std::string_view json_exts[] {".json"};
            constexpr std::string_view bson_exts[] {".bson"};
            constexpr std::string_view yaml_exts[] {".yml", ".yaml"};

            std::string ext = std::filesystem::path(path).extension().string();
            const auto predicate = [&](std::string_view json_ext) { return json_ext == ext; };

            if (std::any_of(std::begin(json_exts), std::end(json_exts), predicate))
            {
                return json_file_type::json;
            }

            if (std::any_of(std::begin(bson_exts), std::end(bson_exts), predicate))
            {
                return json_file_type::bson;
            }

            if (std::any_of(std::begin(yaml_exts), std::end(yaml_exts), predicate))
            {
                return json_file_type::yaml;
            }

            return json_file_type::none;
        }

        inline bool create_directories(std::string_view path)
        {
            std::filesystem::path parent = std::filesystem::path(path).parent_path();

            if (!parent.empty() && !std::filesystem::exists(parent) && !std::filesystem::create_directories(parent))
            {
                return false;
            }

            return true;
        }
    }

    inline bool write_file(std::string_view path, const std::string& content)
    {
        if (!detail::create_directories(path))
        {
            return false;
        }

        std::ofstream stream(path.data());
        stream << content;
        stream.close();
        return !stream.bad();
    }

    inline bool write_file(std::string_view path, const std::vector<std::uint8_t>& bytes)
    {
        if (!detail::create_directories(path))
        {
            return false;
        }

        std::ofstream stream(path.data(), std::ios::out | std::ios::binary);
        stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        stream.close();
        return !stream.bad();
    }

    inline bool write_file(std::string_view path, const nlohmann::json& json)
    {
        constexpr std::size_t indent = 2;
        switch (detail::get_json_type(path))
        {
            case detail::json_file_type::json: {
                return write_file(path, json.dump(indent));
            }

            case detail::json_file_type::bson: {
                return write_file(path, nlohmann::json::to_bson(json));
            }

            case detail::json_file_type::yaml: {
                return write_file(path, yaml::to_yaml(json, indent));
            }

            default: {
                return false;
            }
        }
    }

    inline bool read_file(std::string_view path, std::string& content)
    {
        std::ifstream stream(path.data());
        if (!stream.is_open())
        {
            return false;
        }

        std::stringstream string;
        string << stream.rdbuf();
        content = string.str();
        return !stream.bad();
    }

    inline bool read_file(std::string_view path, std::vector<std::uint8_t>& bytes)
    {
        std::ifstream stream(path.data(), std::ios::in | std::ios::binary);
        if (!stream.is_open())
        {
            return false;
        }

        bytes = {
            std::istreambuf_iterator<char>(stream),
            std::istreambuf_iterator<char>(),
        };

        stream.close();
        return !stream.bad();
    }

    inline bool read_file(std::string_view path, nlohmann::json& json)
    {
        json = nlohmann::json(nlohmann::json::value_t::discarded);
        switch (detail::get_json_type(path))
        {
            case detail::json_file_type::json: {
                std::string value;
                if (read_file(path, value))
                {
                    json = nlohmann::json::parse(value, nullptr, false, false);
                }

                break;
            }

            case detail::json_file_type::bson: {
                std::vector<std::uint8_t> value;
                if (read_file(path, value))
                {
                    json = nlohmann::json::from_bson(value, true, false);
                }

                break;
            }

            case detail::json_file_type::yaml: {
                std::string value;
                if (read_file(path, value))
                {
                    json = yaml::from_yaml(value);
                }

                break;
            }

            default: {
                break;
            }
        }

        return !json.is_discarded();
    }

    inline bool hash_file(std::string_view path, std::string& hash)
    {
        std::vector<std::uint8_t> data;
        if (!read_file(path, data))
        {
            return false;
        }

        picosha2::hash256_hex_string(data.begin(), data.end(), hash);
        return true;
    }
}
