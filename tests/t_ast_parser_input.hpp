#pragma once

#include <string>
#include <optional>

namespace _namespace1::_namespace2
{
  template <typename T>
  using optional_alias_t = std::optional<T>;

    enum class [[_enum_attribute(key1 = value1, key2)]] _enum {
        _value1 [[_enum_value_attribute]],
        _value2 = 42,
    };

    struct _base
    {
    };

    struct [[_struct_attribute]] _struct : _base
    {
        [[_field_attribute]] int _i = 42;
        std::string _s;
        std::optional<int> _opt;
        optional_alias_t<int> _opt_alias;

        _struct() = default;

        [[_constructor_attribute]] _struct([[_argument_attribute]] int _arg)
        {
        }

        [[_func_attribute]] int _member_func([[_argument_attribute]] int _arg)
        {
            return 0;
        }

        template <typename _value_t, int _n = 42>
        [[_func_attribute]] _value_t _template_member_func([[_argument_attribute]] _value_t _arg)
        {
            return _value_t();
        }

        struct _inner
        {
        };
    };

    template <typename _value_t, int _n = 42>
    struct [[_struct_attribute]] _struct_template : _base
    {
        [[_field_attribute]] _value_t _value;

        template <typename _inner_value_t>
        struct _inner_template
        {
        };
    };

    [[_func_attribute]] int _free_func([[_argument_attribute]] int _arg = 42)
    {
        return 0;
    }

    template <typename _value_t, int _n = 42>
    [[_func_attribute]] _value_t _template_free_func([[_argument_attribute]] _value_t _arg)
    {
        return _value_t();
    }
}

void _global_func()
{
}
