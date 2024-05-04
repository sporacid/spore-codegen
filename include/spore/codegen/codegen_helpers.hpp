#pragma once

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <string_view>

#include "fmt/format.h"
#include "picosha2.h"
#include "process.hpp"

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
        stream.close();
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

        hash = picosha2::hash256_hex_string(file_content.begin(), file_content.end());
        return true;
    }

    bool starts_with(const std::string& string, const std::string& prefix)
    {
        return string.size() >= prefix.size() && 0 == string.compare(0, prefix.size(), prefix);
    }

    bool ends_with(const std::string& string, const std::string& suffix)
    {
        return string.size() >= suffix.size() && 0 == string.compare(string.size() - suffix.size(), suffix.size(), suffix);
    }

    void replace_all(std::string& str, const std::string& from, const std::string& to)
    {
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos)
        {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    }

    template <typename container_t, typename projection_t>
    std::string join(std::string_view separator, const container_t& values, const projection_t& projection)
    {
        std::string string;
        for (const auto& value : values)
        {
            string += projection(value);
            string += separator;
        }

        if (string.size() >= separator.size())
        {
            string.resize(string.size() - separator.size());
        }

        return string;
    }

    std::tuple<std::int32_t, std::string, std::string> run(const std::string& command, const std::string& input = std::string {})
    {
        const bool has_stdin = not input.empty();
        const auto reader_factory = [](std::stringstream& stream) {
            return [&](const char* bytes, std::size_t n) {
                stream.write(bytes, static_cast<std::streamsize>(n));
            };
        };

        std::stringstream _stdout;
        std::stringstream _stderr;

        TinyProcessLib::Process process {
            command,
            std::string(),
            reader_factory(_stdout),
            reader_factory(_stderr),
            has_stdin,
        };

        if (has_stdin)
        {
            process.write(input);
            process.close_stdin();
        }

        std::int32_t result = process.get_exit_status();
        return std::tuple {result, _stdout.str(), _stderr.str()};
    }
}
