#pragma once

#include "spore/codegen/formatters/codegen_formatter.hpp"

namespace spore::codegen
{
    struct codegen_formatter_cpp final : codegen_formatter
    {
        bool can_format_file(const std::string_view file) const override;
        bool format_file(const std::string_view file, std::string& file_data) override;
    };
}