#pragma once

#include <memory>
#include <string>

#include <chaiscript/chaiscript.hpp>

#include "spdlog/spdlog.h"

#include "spore/codegen/scripts/codegen_script.hpp"
#include "spore/codegen/scripts/codegen_script_compiler.hpp"

namespace spore::codegen
{
    struct codegen_script_compiler_chaiscript : codegen_script_compiler
    {
        bool compile_script(const std::string& file, codegen_script& script) override
        {
            std::shared_ptr<chaiscript::ChaiScript> chaiscript = std::make_shared<chaiscript::ChaiScript>();
            try
            {
                SPDLOG_DEBUG("compiling chai script, file={}", file);
                chaiscript::Boxed_Value value = chaiscript->eval_file(file);
                return true;
            }
            catch (const chaiscript::exception::eval_error& error)
            {
                SPDLOG_ERROR("failed to compile chai script, file={} error={}", file, error.what());
                return false;
            }
        }
    };
}