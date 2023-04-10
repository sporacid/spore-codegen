#pragma once

#include <string>

namespace _namespace1::_namespace2
{
    enum class [[_enum_attribute(key1 = value1, key2)]] _enum
    {
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

        [[_member_func_attribute]] int _member_func([[_argument_attribute]] int _arg)
        {
            return 0;
        }

        template <typename _value_t>
        [[_template_member_func_attribute]] _value_t _template_member_func([[_template_argument_attribute]] _value_t _arg)
        {
            return _value_t();
        }
    };

    template <typename _value_t>
    struct [[struct_template_attribute]] _struct_template
    {
        _value_t _v;
    };

    [[_free_func_attribute]] int _free_func(int _arg = 42)
    {
        return 0;
    }

    template <typename _value_t>
    [[_template_free_func_attribute]] _value_t _template_free_func(_value_t _arg)
    {
        return _value_t();
    }
}

[[_global_func_attribute]] int _global_func(int)
{
    return 0;
}