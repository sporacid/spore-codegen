#pragma once

#include <map>
#include <string>

namespace spore::codegen
{
    struct slang_attribute
    {
        std::string type;
        std::map<std::string, std::string> values;
    };
}