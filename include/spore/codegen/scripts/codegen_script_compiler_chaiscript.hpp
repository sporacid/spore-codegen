#pragma once

#include <memory>
#include <string>

#include "chaiscript/chaiscript.hpp"
// #include "chaiscript/extras/math.hpp"
// #include "chaiscript/extras/string_methods.hpp"
#include "spdlog/spdlog.h"

#include "spore/codegen/scripts/codegen_script.hpp"
#include "spore/codegen/scripts/codegen_script_compiler.hpp"

namespace spore::codegen
{
    struct codegen_script_compiler_chaiscript : codegen_script_compiler
    {
#if 0
        std::shared_ptr<chaiscript::ChaiScript> chai_script;

        codegen_script_compiler_chaiscript()
            : chai_script(std::make_shared<chaiscript::ChaiScript>())
        {
        }

        bool compile_script(const std::string& file, codegen_script& script) override
        {
            std::string module_ = file;

            for (char character : "<>{}()-+=,.;:/\\ ")
            {
                std::replace(module_.begin(), module_.end(), character, '_');
            }

            SPDLOG_DEBUG("loading chai script module, file={} module={}", file, module_);
            try
            {
                chai_script->load_module(module_, file);
                chai_script->
                return true;
            }
            catch (const chaiscript::exception::load_module_error& error)
            {
                SPDLOG_ERROR("failed to load chai script module, file={} module={} error={}", file, module_, error.what());
                return false;
            }
        }
#else
        bool compile_script(const std::string& file, codegen_script& script) override
        {
            std::shared_ptr<chaiscript::ChaiScript> chai_script = std::make_shared<chaiscript::ChaiScript>();
            // chai_script->add(chaiscript::extras::math::bootstrap());
            // chai_script->add(chaiscript::string_methods::math::bootstrap());

            try
            {
                SPDLOG_DEBUG("compiling chai script, file={}", file);
                chai_script->eval_file(file);
                auto s = chai_script->eval("say_hello(\"wtf\")");
                return true;
                // chaiscript::Boxed_Value value = chai_script->eval_file(file);

//                chai_script->load_module("module", file);
//
//                if (value.is_undef() || value.is_null())
//                {
//                    SPDLOG_ERROR("failed to compile chai script, file={}", file);
//                    return false;
//                }

                // TODO populate script struct

//                return !value.is_undef() && !value.is_null();
            }
            catch (const chaiscript::exception::eval_error& error)
            {
                SPDLOG_ERROR("failed to compile chai script, file={} error={}", file, error.what());
                return false;
            }
        }
#endif
    };
}