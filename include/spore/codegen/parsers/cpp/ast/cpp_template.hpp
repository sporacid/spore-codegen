#pragma once

#include <optional>
#include <string>
#include <vector>

namespace spore::codegen
{
    struct cpp_template_param
    {
        std::string type;
        std::string name;
        std::optional<std::string> default_value;
        bool is_variadic = false;
    };

    template <typename cpp_value_t>
    struct cpp_has_template_params
    {
        std::vector<cpp_template_param> template_params;
        std::vector<std::string> template_specialization_params;

        [[nodiscard]] bool is_template() const
        {
            return !template_params.empty();
        }

        [[nodiscard]] bool is_template_specialization() const
        {
            return !template_specialization_params.empty();
        }
    };
}