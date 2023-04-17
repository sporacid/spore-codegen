#pragma once

#include <string>

namespace _namespace1::_namespace2
{
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
    };

    template <typename _value_t, int _n = 42>
    struct [[_struct_attribute]] _struct_template : _base
    {
        [[_field_attribute]] _value_t _value;
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
