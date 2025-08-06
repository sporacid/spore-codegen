#pragma once

#if defined(__clang__)
#    define SPORE_CODEGEN_PUSH_DISABLE_WARNINGS                 \
        _Pragma("clang diagnostic push")                        \
            _Pragma("clang diagnostic ignored \"-Wall\"")       \
                _Pragma("clang diagnostic ignored \"-Wextra\"") \
                    _Pragma("clang diagnostic ignored \"-Wpedantic\"")
#    define SPORE_CODEGEN_POP_DISABLE_WARNINGS \
        _Pragma("clang diagnostic pop")
#elif defined(__GNUC__) || defined(__GNUG__)
#    define SPORE_CODEGEN_PUSH_DISABLE_WARNINGS               \
        _Pragma("GCC diagnostic push")                        \
            _Pragma("GCC diagnostic ignored \"-Wall\"")       \
                _Pragma("GCC diagnostic ignored \"-Wextra\"") \
                    _Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#    define SPORE_CODEGEN_POP_DISABLE_WARNINGS \
        _Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
#    define SPORE_CODEGEN_PUSH_DISABLE_WARNINGS \
        __pragma(warning(push, 0))
#    define SPORE_CODEGEN_POP_DISABLE_WARNINGS \
        __pragma(warning(pop))
#else
#    define SPORE_CODEGEN_PUSH_DISABLE_WARNINGS
#    define SPORE_CODEGEN_POP_DISABLE_WARNINGS
#endif
