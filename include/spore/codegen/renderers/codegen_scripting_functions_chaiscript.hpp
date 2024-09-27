#pragma once

#include <string>

#include "chaiscript/chaiscript.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "spore/codegen/renderers/codegen_scripting_functions.hpp"
#include "spore/codegen/utils/strings.hpp"

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

        chaiscript::ChaiScript chai;

        codegen_scripting_functions_chaiscript()
        {
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

        explicit codegen_scripting_functions_chaiscript(std::span<const std::string> files)
        {
            const auto action = [&](const std::string& file) { chai.use(file); };
            std::ranges::for_each(files, action);
        }

        [[nodiscard]] bool invoke(std::string_view function, const nlohmann::json& params, nlohmann::json& result) override
        {
            const auto projection = [&](const auto& param_pair) {
                return fmt::format("from_json(\"{}\")", param_pair.value().dump());
            };

            try
            {
                const std::string script = fmt::format("to_json({}({}));", function, strings::join(", ", params.items(), projection));
                const std::string json = chai.eval<std::string>(script);
                result = nlohmann::json::parse(json);
                return true;
            }
            catch (const chaiscript::exception::eval_error& e)
            {
                SPDLOG_WARN("chaiscript evaluation error: {}", e.pretty_print());
                return false;
            }
        }

        [[nodiscard]] bool can_invoke(std::string_view function, const nlohmann::json& params) const override
        {
            return false;
        }
    };
}