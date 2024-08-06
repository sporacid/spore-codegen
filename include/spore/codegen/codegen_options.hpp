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
        std::vector<std::string> includes;
        std::vector<std::string> template_paths;
        std::vector<std::pair<std::string, std::string>> definitions;
        std::vector<std::pair<std::string, std::string>> user_data;
        bool force = false;
        bool debug = false;
    };
}
