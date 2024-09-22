#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace spore::codegen::aggregates
{
    namespace detail
    {
        template <std::size_t index_v, typename value_t, typename func_t>
        constexpr void for_each_impl(value_t&& value, func_t&& func)
        {
            if constexpr (index_v < std::tuple_size_v<std::decay_t<value_t>>)
            {
                func(std::get<index_v>(std::forward<value_t>(value)));
                for_each_impl<index_v + 1>(std::forward<value_t>(value), std::forward<func_t>(func));
            }
        }
    }

    template <typename value_t, typename func_t>
    constexpr auto for_each(value_t&& value, func_t&& func)
    {
        detail::for_each_impl<0>(std::forward<value_t>(value), std::forward<func_t>(func));
    }
}