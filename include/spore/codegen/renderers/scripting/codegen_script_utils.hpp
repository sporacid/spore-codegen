#pragma once

#include <optional>
#include <ranges>
#include <tuple>

#include "nlohmann/json.hpp"

namespace spore::codegen::scripts
{
    namespace detail
    {
        template <typename value_t>
        struct is_optional : std::false_type
        {
        };

        template <typename value_t>
        struct is_optional<std::optional<value_t>> : std::true_type
        {
        };

        template <std::size_t index_v, typename... params_t>
        void get_params_impl(std::tuple<params_t...>& params, const std::span<const nlohmann::json> json_params)
        {
            if constexpr (index_v < sizeof...(params_t))
            {
                using param_t = std::decay_t<decltype(std::get<index_v>(params))>;
                param_t& param = std::get<index_v>(params);

                if (index_v < json_params.size())
                {
                    if constexpr (is_optional<param_t>::value)
                    {
                        param.emplace();
                        json_params[index_v].get_to(param.value());
                    }
                    else
                    {
                        json_params[index_v].get_to(param);
                    }
                }
            }
        }
    }

    template <typename... params_t>
    std::tuple<params_t...> get_params(const std::span<const nlohmann::json> json_params)
    {
        std::tuple<params_t...> params;
        detail::get_params_impl<0>(params, json_params);
        return params;
    }
}