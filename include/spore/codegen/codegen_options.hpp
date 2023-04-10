#pragma once

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
        std::vector<std::string> includes;
        std::vector<std::string> features;
        std::vector<std::pair<std::string, std::string>> definitions;
        bool force = false;
        bool reformat = false;
        bool sequential = false;
    };
}