#pragma once

#include <string>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

namespace spore::codegen
{
    struct codegen_options
    {
        std::string config;
        std::string cache;
        std::vector<std::string> templates;
        std::vector<std::pair<std::string, nlohmann::json>> user_data;
        bool reformat : 1 = false;
        bool force : 1 = false;
        bool debug : 1 = false;
    };
}
