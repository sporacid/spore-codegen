#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace spore::codegen
{
    struct codegen_options
    {
        std::string output;
        std::string config;
        std::string cache;
        std::string cpp_standard;
        std::string reformat;
        std::string clang;
        std::vector<std::string> includes;
        std::vector<std::string> features;
        std::vector<std::string> template_paths;
        std::vector<std::pair<std::string, std::string>> definitions;
        std::vector<std::pair<std::string, std::string>> user_data;
        std::optional<std::string> dump_ast = std::nullopt;
        std::size_t parallelism = 0;
        bool force = false;
    };
}
