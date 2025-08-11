#include "spore/codegen/formatters/codegen_formatter_cpp.hpp"

#include <filesystem>

#include "spore/codegen/codegen_macros.hpp"
#include "spore/codegen/codegen_version.hpp"

#include "spdlog/spdlog.h"

SPORE_CODEGEN_PUSH_DISABLE_WARNINGS
#include "clang/Format/Format.h"
SPORE_CODEGEN_POP_DISABLE_WARNINGS

namespace spore::codegen
{
    bool codegen_formatter_cpp::can_format_file(const std::string_view file) const
    {
        constexpr std::string_view extensions[] {".h", ".c", ".hpp", ".cpp", ".cppm", ".cxx", ".cc", ".c++", ".tpp", ".inl"};
        const std::string extension = std::filesystem::path(file).extension().generic_string();
        return std::ranges::find(extensions, extension) != std::end(extensions);
    }

    bool codegen_formatter_cpp::format_file(const std::string_view file, std::string& file_data)
    {
        llvm::Expected<clang::format::FormatStyle> format_style = clang::format::getStyle("file", file, clang::format::DefaultFallbackStyle);
        if (not format_style)
        {
            SPDLOG_DEBUG("clang-format style not found, path={}", file);
            return false;
        }

        auto apply_reformat = [&]<typename func_t>(func_t&& func) {
            const std::array<clang::tooling::Range, 1> ranges {
                clang::tooling::Range {
                    static_cast<unsigned>(0),
                    static_cast<unsigned>(file_data.size()),
                },
            };

            const clang::tooling::Replacements replacements = func(*format_style, file_data, ranges, file);

            llvm::Expected<std::string> new_file_data = clang::tooling::applyAllReplacements(file_data, replacements);
            if (not new_file_data)
            {
                SPDLOG_DEBUG("clang format failed, path={}", file);
                return false;
            }

            std::ignore = std::move(new_file_data).moveInto(file_data);
            return true;
        };

        return apply_reformat([]<typename... args_t>(args_t&&... args) { return clang::format::sortIncludes(std::forward<args_t>(args)...); }) and
               apply_reformat([]<typename... args_t>(args_t&&... args) { return clang::format::reformat(std::forward<args_t>(args)...); });
    }
}