#pragma once

#include <filesystem>

namespace spore::codegen
{
    struct current_path_scope
    {
        std::filesystem::path old_path;

        explicit current_path_scope(const std::filesystem::path& new_path)
        {
            old_path = std::filesystem::current_path();
            std::filesystem::current_path(new_path);
        }

        ~current_path_scope()
        {
            std::filesystem::current_path(old_path);
        }

        current_path_scope(const current_path_scope&) = delete;
        current_path_scope(current_path_scope&&) = delete;

        current_path_scope& operator=(const current_path_scope&) = delete;
        current_path_scope& operator=(current_path_scope&&) = delete;
    };
}