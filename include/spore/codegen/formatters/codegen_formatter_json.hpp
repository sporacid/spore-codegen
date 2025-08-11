#pragma once

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "spore/codegen/formatters/codegen_formatter.hpp"

namespace spore::codegen
{
    struct codegen_formatter_json final : codegen_formatter
    {
        std::size_t indent = 2;

        bool can_format_file(const std::string_view file) const override
        {
            return ".json" == std::filesystem::path(file).extension();
        }

        bool format_file(const std::string_view file, std::string& file_data) override
        {
            const nlohmann::json json = nlohmann::json::parse(file_data);

            if (json.is_discarded())
            {
                SPDLOG_DEBUG("invalid JSON, path={}", file);
                return false;
            }

            file_data = json.dump(indent);
            return true;
        }
    };
}