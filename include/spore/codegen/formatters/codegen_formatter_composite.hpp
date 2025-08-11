#pragma once

#include <memory>
#include <unordered_map>

#include "spdlog/spdlog.h"

#include "spore/codegen/formatters/codegen_formatter.hpp"

namespace spore::codegen
{
    struct codegen_formatter_composite final : codegen_formatter
    {
        std::vector<std::shared_ptr<codegen_formatter>> formatters;

        template <typename... formatters_t>
        explicit codegen_formatter_composite(formatters_t&&... formatters)
            : formatters {std::forward<formatters_t>(formatters)...}
        {
        }
        bool can_format_file(const std::string_view file) const override

        {
            const auto predicate = [&](const std::shared_ptr<codegen_formatter>& formatter) {
                return formatter->can_format_file(file);
            };

            return std::ranges::any_of(formatters, predicate);
        }

        bool format_file(const std::string_view file, std::string& file_data) override
        {
            const auto predicate = [&](const std::shared_ptr<codegen_formatter>& formatter) {
                return formatter->can_format_file(file);
            };

            const auto it_formatter = std::ranges::find_if(formatters, predicate);
            if (it_formatter != formatters.end())
            {
                const std::shared_ptr<codegen_formatter>& formatter = *it_formatter;
                return formatter->format_file(file, file_data);
            }

            return false;
        }
    };
}