#pragma once

#include <string>
#include <string_view>

namespace spore::codegen
{
    struct codegen_formatter
    {
        virtual ~codegen_formatter() = default;
        [[nodiscard]] virtual bool can_format_file(const std::string_view file) const = 0;
        [[nodiscard]] virtual bool format_file(const std::string_view file, std::string& file_data) = 0;
    };
}