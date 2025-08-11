#pragma once

#include <sstream>

#include "spdlog/spdlog.h"
#include "yaml-cpp/yaml.h"

#include "spore/codegen/formatters/codegen_formatter.hpp"

namespace spore::codegen
{
    struct codegen_formatter_yaml final : codegen_formatter
    {
        std::size_t indent = 2;

        bool can_format_file(const std::string_view file) const override
        {
            constexpr std::string_view extensions[] {".yaml", ".yml"};
            const std::string extension = std::filesystem::path(file).extension().generic_string();
            return std::ranges::find(extensions, extension) != std::end(extensions);
        }

        bool format_file(const std::string_view file, std::string& file_data) override
        {
            const YAML::Node yaml = YAML::Load(file_data);

            if (not yaml.IsDefined())
            {
                SPDLOG_DEBUG("invalid YAML, path={}", file);
                return false;
            }

            std::stringstream stream {std::move(file_data)};

            YAML::Emitter emitter {stream};
            emitter.SetIndent(indent);
            emitter << yaml;

            file_data = std::move(stream).str();

            return true;
        }
    };
}