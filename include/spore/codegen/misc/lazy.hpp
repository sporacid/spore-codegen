#pragma once

#include <functional>
#include <variant>

#include "nlohmann/json.hpp"

#include "spore/codegen/codegen_error.hpp"

namespace spore::codegen
{
    template <typename value_t>
    struct lazy
    {
        lazy() = default;

        lazy(const lazy&) noexcept = default;
        lazy(lazy&&) noexcept = default;

        template <typename arg_t>
        lazy(arg_t&& arg)
            : _variant(std::forward<arg_t>(arg))
        {
        }

        lazy& operator=(const lazy&) noexcept = default;
        lazy& operator=(lazy&&) noexcept = default;

        template <typename arg_t>
        lazy& operator=(arg_t&& arg)
        {
            _variant = std::forward<arg_t>(arg);
            return *this;
        }

        bool valid() const
        {
            return !std::holds_alternative<std::monostate>(_variant);
        }

        void reset()
        {
            _variant = std::monostate();
        }

        value_t& get()
        {
            check_valid();

            if (std::holds_alternative<func_t>(_variant))
            {
                const func_t& func = std::get<func_t>(_variant);
                _variant = func();
            }

            return std::get<value_t>(_variant);
        }

        const value_t& get() const
        {
            return const_cast<lazy&>(*this).get();
        }

        value_t* operator->()
        {
            return &get();
        }

        const value_t* operator->() const
        {
            return &get();
        }

        value_t& operator*()
        {
            return get();
        }

        const value_t& operator*() const
        {
            return get();
        }

      private:
        using func_t = std::function<value_t()>;
        std::variant<std::monostate, func_t, value_t> _variant;

        void check_valid() const
        {
            if (!valid()) [[unlikely]]
            {
                throw codegen_error(codegen_error_code::invalid, "invalid lazy state");
            }
        }
    };

    template <typename value_t>
    void to_json(nlohmann::json& json, const lazy<value_t>& lazy)
    {
        json = lazy.get();
    }

    template <typename value_t>
    void from_json(const nlohmann::json& json, lazy<value_t>& lazy)
    {
        value_t value;
        json.get_to(value);
        lazy = std::move(value);
    }
}