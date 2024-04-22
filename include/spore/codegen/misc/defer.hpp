#pragma once

#include <optional>
#include <tuple>

namespace spore::codegen
{
    template <typename func_t, typename... args_t>
    struct defer
    {
        defer() = default;

        defer(defer&&) noexcept = delete;
        defer& operator=(defer&&) noexcept = delete;

        defer(const defer&) noexcept = delete;
        defer& operator=(const defer&) noexcept = delete;

        defer(func_t func, args_t... args)
        {
            bind(std::forward<func_t&&>(func), std::forward<args_t&&>(args)...);
        }

        ~defer() noexcept
        {
            reset();
        }

        void bind(func_t func, args_t... args)
        {
            bind(std::forward<func_t&&>(func), std::tuple<args_t...>(std::forward<args_t&&>(args)...));
        }

        void bind(func_t func, std::tuple<args_t...> args)
        {
            if (_func.has_value())
            {
                std::apply(_func.value(), std::move(_args));
            }

            _func.emplace(std::forward<func_t&&>(func));
            _args = std::move(args);
        }

        void reset()
        {
            if (_func.has_value())
            {
                std::apply(_func.value(), std::move(_args));
            }

            _func.reset();
            _args = {};
        }

      private:
        std::optional<func_t> _func;
        std::tuple<args_t...> _args;
    };
}