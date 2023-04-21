#pragma once

#include <string>

#include "chaiscript/chaiscript.hpp"
#include "spdlog/spdlog.h"

#include "spore/codegen/scripts/codegen_script.hpp"
#include "spore/codegen/scripts/codegen_script_compiler.hpp"

namespace spore::codegen
{
    struct codegen_script_compiler_chaiscript : codegen_script_compiler
    {
        chaiscript::ChaiScript chaiscript;

        bool compile_script(const std::string& file, codegen_script& script) override
        {
            try
            {
                SPDLOG_DEBUG("compiling chai script, file={}", file);
                chaiscript::Boxed_Value value = chaiscript.eval_file(file);
            }
            catch(const chaiscript::exception::eval_error& e)
            {

            }

            return false;
        }
    };
}