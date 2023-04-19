#pragma once

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <string_view>

#include "cppast/compile_config.hpp"
#include "fmt/format.h"

namespace spore::codegen
{
    bool write_file(const std::string_view& path, const std::string& content)
    {
        std::filesystem::path parent = std::filesystem::path(path).parent_path();

        if (!parent.empty() && !std::filesystem::exists(parent) && !std::filesystem::create_directories(parent))
        {
            return false;
        }

        std::ofstream stream(path.data());
        stream << content;
        return !stream.bad();
    }

    bool read_file(const std::string_view& path, std::string& content)
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

    bool hash_file(const std::string_view& path, std::string& hash)
    {
        std::string file_content;
        if (!spore::codegen::read_file(path, file_content))
        {
            return false;
        }

        std::uint64_t hash_code = std::hash<std::string>()(file_content);

        std::stringstream string;
        string << std::setw(16) << std::setfill('0') << std::hex << hash_code;
        hash = string.str();

        return true;
    }

    cppast::cpp_standard parse_cpp_standard(const std::string_view& string)
    {
        static const std::map<std::string_view, cppast::cpp_standard> cpp_standard_map {
            {cppast::to_string(cppast::cpp_standard::cpp_98), cppast::cpp_standard::cpp_98},
            {cppast::to_string(cppast::cpp_standard::cpp_03), cppast::cpp_standard::cpp_03},
            {cppast::to_string(cppast::cpp_standard::cpp_11), cppast::cpp_standard::cpp_11},
            {cppast::to_string(cppast::cpp_standard::cpp_14), cppast::cpp_standard::cpp_14},
            {cppast::to_string(cppast::cpp_standard::cpp_1z), cppast::cpp_standard::cpp_1z},
            {cppast::to_string(cppast::cpp_standard::cpp_17), cppast::cpp_standard::cpp_17},
            {cppast::to_string(cppast::cpp_standard::cpp_2a), cppast::cpp_standard::cpp_2a},
            {cppast::to_string(cppast::cpp_standard::cpp_20), cppast::cpp_standard::cpp_20},
        };

        const auto it = cpp_standard_map.find(string);
        return it != cpp_standard_map.end() ? it->second : cppast::cpp_standard::cpp_latest;
    }

    bool starts_with(const std::string& string, const std::string& prefix)
    {
        return string.size() >= prefix.size() && 0 == string.compare(0, prefix.size(), prefix);
    }

    bool ends_with(const std::string& string, const std::string& suffix)
    {
        return string.size() >= suffix.size() && 0 == string.compare(string.size() - suffix.size(), suffix.size(), suffix);
    }
}