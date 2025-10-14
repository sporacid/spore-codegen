#pragma once

#include <cstdint>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace spore::codegen::strings
{
    template <typename callback_t>
    inline void for_each_words(const std::string_view input, const callback_t&& callback);

    namespace detail
    {
        using char_replace_func = std::function<std::string::value_type(std::string::value_type)>;

        inline std::string::value_type tolower(std::string::value_type c)
        {
            return std::tolower(c);
        }

        inline std::string::value_type toupper(std::string::value_type c)
        {
            return std::toupper(c);
        }

        inline std::string to_any_case(
            const std::string_view input,
            const std::string_view separator,
            const char_replace_func first_word_first_char_func,
            const char_replace_func word_first_char_func,
            const char_replace_func word_char_func)
        {
            constexpr std::size_t average_word_count_in_sentence = 10;
            std::string result;

            result.reserve(input.size() + average_word_count_in_sentence * separator.size());

            bool is_first_word = true;

            for_each_words(input, [&](const std::string_view word) {
                if (!is_first_word)
                {
                    result += separator;
                }

                bool is_first_char_in_word = true;

                for (const std::string::value_type c : word)
                {
                    if (is_first_char_in_word)
                    {
                        if (is_first_word)
                        {
                            result += first_word_first_char_func(c);
                        }
                        else
                        {
                            result += word_first_char_func(c);
                        }

                        is_first_char_in_word = false;
                    }
                    else
                    {
                        result += word_char_func(c);
                    }
                }

                is_first_word = false;
            });

            return result;
        }
    }

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

    template <typename callback_t>
    inline void for_each_words(const std::string_view input, const callback_t&& callback)
    {
        std::optional<std::string_view::const_iterator> word_begin;

        for (auto it = input.cbegin(); it < input.cend(); ++it)
        {
            if (word_begin)
            {
                if (std::isalnum(*it))
                {
                    if (std::isupper(*it))
                    {
                        const bool is_previous_lower = std::islower(*(it - 1));
                        const bool is_last = it == input.cend() - 1;
                        const bool is_next_lower = is_last ? false : std::islower(*(it + 1));
                        const bool is_next_last = is_last ? false : it + 1 == input.cend() - 1;
                        if (is_previous_lower || (is_next_lower && !is_next_last))
                        {
                            callback(std::string_view(word_begin.value(), it));
                            word_begin = it;
                        }
                    }
                }
                else
                {
                    callback(std::string_view(word_begin.value(), it));
                    word_begin = std::nullopt;
                }
            }
            else
            {
                if (std::isalnum(*it))
                {
                    word_begin = it;
                }
            }
        }

        if (word_begin)
        {
            callback(std::string_view(word_begin.value(), input.cend()));
        }
    }

    inline std::vector<std::string_view> split_into_words(const std::string_view input)
    {
        std::vector<std::string_view> result;

        for_each_words(input, [&](const std::string_view word) {
            result.push_back(word);
        });

        return result;
    }

    inline std::string to_camel_case(const std::string_view input)
    {
        return detail::to_any_case(input, "", detail::tolower, detail::toupper, detail::tolower);
    }

    inline std::string to_upper_camel_case(const std::string_view input)
    {
        return detail::to_any_case(input, "", detail::toupper, detail::toupper, detail::tolower);
    }

    inline std::string to_snake_case(const std::string_view input)
    {
        return detail::to_any_case(input, "_", detail::tolower, detail::tolower, detail::tolower);
    }

    inline std::string to_upper_snake_case(const std::string_view input)
    {
        return detail::to_any_case(input, "_", detail::toupper, detail::toupper, detail::toupper);
    }

    inline std::string to_title_case(const std::string_view input)
    {
        return detail::to_any_case(input, " ", detail::toupper, detail::toupper, detail::tolower);
    }

    inline std::string to_sentence_case(const std::string_view input)
    {
        return detail::to_any_case(input, " ", detail::toupper, detail::tolower, detail::tolower);
    }
}

template <typename string_t>
struct std::formatter<std::vector<string_t>> : formatter<std::string>
{
    auto format(const std::vector<string_t>& values, format_context& ctx) const
    {
        using namespace spore::codegen;
        std::string result = "[" + strings::join(",", values) + "]";
        return formatter<std::string>::format(std::move(result), ctx);
    }
};
