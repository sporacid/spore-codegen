#pragma once

#include <memory>
#include <string>

#include "nlohmann/json.hpp"

#include "spore/codegen/scripts/codegen_script.hpp"

namespace spore::codegen
{
    struct codegen_script_compiler : std::enable_shared_from_this<codegen_script_compiler>
    {
        virtual ~codegen_script_compiler() = default;
        virtual bool compile_script(const std::string& file, codegen_script& script);
    };
}