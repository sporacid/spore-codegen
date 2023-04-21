#pragma once

#include <memory>
#include <string>

#include "nlohmann/json.hpp"

#include "spore/codegen/scripts/codegen_script.hpp"

namespace spore::codegen
{
    struct codegen_script_provider : std::enable_shared_from_this<codegen_script_provider>
    {
        virtual ~codegen_script_provider() = default;
        virtual bool install_script(codegen_script& script);
    };
}