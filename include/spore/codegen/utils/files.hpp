#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include "picosha2.h"

namespace spore::codegen::files
{
    inline bool write_file(const std::string_view& path, const std::string& content)
    {
        std::filesystem::path parent = std::filesystem::path(path).parent_path();

        if (!parent.empty() && !std::filesystem::exists(parent) && !std::filesystem::create_directories(parent))
        {
            return false;
        }

        std::ofstream stream(path.data());
        stream << content;
        stream.close();
        return !stream.bad();
    }

    inline bool read_file(const std::string_view& path, std::string& content)
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

    inline bool hash_file(const std::string_view& path, std::string& hash)
    {
        std::string file_content;
        if (!read_file(path, file_content))
        {
            return false;
        }

        hash = picosha2::hash256_hex_string(file_content.begin(), file_content.end());
        return true;
    }
}
