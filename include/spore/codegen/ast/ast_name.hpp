#pragma once

#include <string>

#include "spore/codegen/ast/ast_template.hpp"

namespace spore::codegen
{
    template <typename ast_value_t>
    struct ast_has_name
    {
        std::string scope;
        std::string name;

        [[nodiscard]] std::string full_name() const
        {
            std::string full_name;

            if (!scope.empty())
            {
                full_name += scope + "::";
            }

            full_name += name;

            if constexpr (std::is_convertible_v<ast_value_t, ast_has_template_params<ast_value_t>>)
            {
                const auto& has_template_params = static_cast<const ast_value_t&>(*this);

                if (has_template_params.is_template() || has_template_params.is_template_specialization())
                {
                    full_name += "<";

                    if (has_template_params.is_template())
                    {
                        for (const ast_template_param& template_param : has_template_params.template_params)
                        {
                            full_name += template_param.name + ", ";
                        }
                    }
                    else if (has_template_params.is_template_specialization())
                    {
                        for (const std::string& template_specialization_param : has_template_params.template_specialization_params)
                        {
                            full_name += template_specialization_param + ", ";
                        }
                    }

                    if (full_name.ends_with(", "))
                    {
                        full_name.resize(full_name.length() - 2);
                    }

                    full_name += ">";
                }
            }

            return full_name;
        }

        bool operator==(const ast_has_name& other) const
        {
            return scope == other.scope && name == other.name;
        }
    };
}