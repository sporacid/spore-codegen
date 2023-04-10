#pragma once

#include <string>

namespace spore::codegen
{
    enum class ast_include_type
    {
        unknown,
        system,
        local,
    };

    struct ast_include
    {
        ast_include_type type = ast_include_type::unknown;
        std::string name;
        std::string path;
    };
}