#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace spore::codegen
{
    struct codegen_options
    {
        // std::string output;
        std::string config;
        std::string cache;
        std::string reformat;
        std::vector<std::string> templates;
        std::vector<std::pair<std::string, std::string>> user_data;
        std::map<std::string_view, std::vector<std::string>> additional_args;
        bool force = false;
        bool debug = false;
    };
}
