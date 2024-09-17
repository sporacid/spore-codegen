#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "fmt/format.h"

namespace spore::codegen::strings
{
    constexpr std::string_view whitespaces = " \t\r\n";

    inline void replace_all(std::string& str, const std::string_view from, const std::string_view to)
    {
        std::size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos)
        {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    }

    template <typename container_t, typename projection_t = std::identity>
    std::string join(const std::string_view separator, const container_t& values, const projection_t& projection = {})
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

    template <typename string_t>
    void trim_start(string_t& value, const std::string_view chars = whitespaces)
    {
        const std::size_t index = value.find_first_not_of(chars);
        value = value.substr(std::min(index, value.size()));
    }

    template <typename string_t>
    void trim_end(string_t& value, const std::string_view chars = whitespaces)
    {
        const std::size_t index = value.find_last_not_of(chars);
        value = value.substr(0, std::max(index + 1, std::size_t {0}));
    }

    template <typename string_t>
    void trim(string_t& value, const std::string_view chars = whitespaces)
    {
        trim_start(value, chars);
        trim_end(value, chars);
    }
}

template <typename string_t>
struct fmt::formatter<std::vector<string_t>> : formatter<std::string>
{
    auto format(const std::vector<string_t>& values, format_context& ctx) const
    {
        using namespace spore::codegen;
        std::string result = "[" + strings::join(",", values) + "]";
        return formatter<std::string>::format(std::move(result), ctx);
    }
};
