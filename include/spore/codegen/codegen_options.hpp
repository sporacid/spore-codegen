#pragma once

#include <string>
#include <utility>
#include <vector>

namespace spore::codegen
{
    struct codegen_options
    {
        std::string config;
        std::string cache;
        std::vector<std::string> templates;
        std::vector<std::string> scripts;
        std::vector<std::pair<std::string, std::string>> user_data;
        bool reformat = false;
        bool force = false;
        bool debug = false;
    };
}
