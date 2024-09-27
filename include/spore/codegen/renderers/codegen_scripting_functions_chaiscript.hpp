#pragma once

#include <string>

#include "chaiscript/chaiscript.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "spore/codegen/renderers/codegen_scripting_functions.hpp"

namespace spore::codegen
{
    struct codegen_scripting_functions_chaiscript final : codegen_scripting_functions
    {
        static constexpr auto script0 = R"(
            namespace("spore")
            spore.print_wtf = fun (x) {
              print("wtf ${x}");
            }
        )";

        static constexpr auto script1 = R"(
            spore.print_wtf("!!1");
        )";

        codegen_scripting_functions_chaiscript()
        {
            chaiscript::ChaiScript chai;
            try
            {
                const auto print = [](const std::string& str) { SPDLOG_INFO("chaiscript: {}", str); };
                chai.add(chaiscript::fun(print), "print");
                const auto result0 = chai.eval(script0);
                const auto result1 = chai.eval(script1);
                printf("");
            }
            catch (const chaiscript::exception::eval_error& e)
            {
                SPDLOG_ERROR("chaiscript: {}", e.pretty_print());
            }
        }

        [[nodiscard]] bool invoke(std::string_view function, const nlohmann::json& params, nlohmann::json& result) override
        {
            return false;
        }

        [[nodiscard]] bool can_invoke(std::string_view function, const nlohmann::json& params) const override
        {
            return false;
        }
    };
}