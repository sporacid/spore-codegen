#pragma once

#define ATTRIBUTE(...) [[clang::annotate(#__VA_ARGS__)]]

namespace _namespace1::_namespace2
{
    enum class ATTRIBUTE(_enum) _enum
    {
        _value1 ATTRIBUTE(_enum_value),
        _value2 = 42,
    };

    struct _base
    {
    };

    struct ATTRIBUTE(_struct) _struct : _base
    {
        ATTRIBUTE(_field)
        int _i = 42;
        float _f;

        ATTRIBUTE(_constructor)
        _struct() = default;

        ATTRIBUTE(_constructor)
        _struct(ATTRIBUTE(_argument) int _arg)
        {
        }

        ATTRIBUTE(_function)
        inline int _member_func(ATTRIBUTE(_argument) int _arg)
        {
            return 0;
        }

        template <typename _value_t, int _n = 42>
        ATTRIBUTE(_function)
        _value_t _template_member_func(ATTRIBUTE(_argument) _value_t _arg)
        {
            return _value_t();
        }

        struct _nested
        {
        };

        enum _nested_enum
        {
        };
    };

    template <typename _value_t, int _n = 42>
    struct ATTRIBUTE(_struct) _struct_template : _base
    {
        ATTRIBUTE(_field)
        _value_t _value;

        template <typename _nested_value_t = int>
        struct ATTRIBUTE(_inner) _nested_template
        {
        };
    };

    ATTRIBUTE(_function)
    int _free_func(ATTRIBUTE(_argument) int _arg = 42)
    {
        return 0;
    }

    template <typename _value_t, int _n = 42>
    ATTRIBUTE(_function)
    _value_t _template_free_func(ATTRIBUTE(_argument) _value_t _arg)
    {
        return _value_t();
    }
}

inline void _global_func()
{
}

struct ATTRIBUTE(a, b = 1, c = "2", d = 3.0, e = (f = 4, g = (h = 5))) _attributes
{
    ATTRIBUTE(a, a = false)
    int _overridden_field = 0;
};

template <typename>
concept concept_ = true;

template <typename value_t, int size_v, template <typename arg_t, typename, int> typename template_t, concept_ concept_t, typename... variadic_t>
struct _template
{
    template <typename... args_t>
    void variadic_func(args_t&&... args)
    {
    }
};

template <typename, int, template <typename, typename, int> typename, concept_, typename...>
struct _unnamed_template
{
};

template <typename...>
struct _template_specialization
{
};

template <typename value_t>
struct _template_specialization<int, float, value_t, _template_specialization<int, float>>
{
};
