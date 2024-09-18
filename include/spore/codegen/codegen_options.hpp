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
        std::vector<std::pair<std::string, std::string>> user_data;
        std::multimap<std::string_view, std::string> additional_args;
        bool force = false;
        bool debug = false;
    };
}
