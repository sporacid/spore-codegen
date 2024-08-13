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
        std::string reformat;
        std::vector<std::string> templates;
        std::vector<std::string> additional_args;
        std::vector<std::pair<std::string, std::string>> user_data;
        bool force = false;
        bool debug = false;
    };
}
