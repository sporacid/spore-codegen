# C++ Parser

`C++` parser is implemented via `libclang` and requires `LLVM` to be installed. To use the `C++` parser, you can
configure the stage's `parser` property to `cpp`:

```yaml
stages:
  - # ...
    parser: cpp
    # ...
```

## Table of Contents

- [Clang Annotate Attributes](#clang-annotate-attributes)
- [Parser Arguments](#parser-arguments)
- [Parser Conditions](#parser-conditions)
- [Implementation Guidelines](#implementation-guidelines)
    - [Preventing Circular Dependencies](#preventing-circular-dependencies)
    - [Preventing Usage Before Declarations](#preventing-usage-before-declarations)
    - [Useful Macros](#useful-macros)

## Clang Annotate Attributes

The `clang::annotate` attribute is used to inject metadata on `C++` symbols. The content of the attribute is parsed into
a
`JSON` object, from a unique syntax to improve usability:

| Usage            | Syntax                                                                     | JSON result                                            | Notes                                                                                        |
|------------------|----------------------------------------------------------------------------|--------------------------------------------------------|----------------------------------------------------------------------------------------------|
| Single Flag      | `flag` or `flag = false`                                                   | `{"flag": true}` or `{"flag": false}`                  | By default, a flag is set to true.                                                           |
| Single Value     | `value = 1` or `value = 1.0` or `value = "1"`                              | `{"value": 1}` or `{"value": 1.0}` or `{"value": "1"}` | Values can be `int`, `float` or `string`. Technically, flags are also values of type `bool`. |
| Single Object    | `object = (flag, value = 1)`                                               | `{"object": {"flag": true, "value": 1}}`               | Objects are denoted with parenthesis.                                                        |
| Overridden Value | `value = 1, value = 2`                                                     | `{"value": 2}`                                         | Subsequent values with the same keys override the previous value.                            |
| Merged Object    | `object = (flag, value = 1), object = (flag = false, value = 2, new = 42)` | `{"object": {"flag": false, "value": 2, "new": 42}}`   | Subsequent objects with the same keys are merged with the previous value.                    |
| Nested Objects   | `object = (nested = (flag))`                                               | `{"object": {"nested": {"flag": true}}}`               | Objects are recursive and can be nested without any limits.                                  |

Here is a simple example using the `clang::annotate` attribute on a struct:

```cpp
struct [[clang::annotate("flag, value = 1")]] data
{
};
```

Here is a more complex example with macro definitions to simplify usage:

```cpp
#define ATTRIBUTE(...) [[clang::annotate(#__VA_ARGS__)]]
#define ENUM(...) ATTRIBUTE(enum, __VA_ARGS__)
#define CLASS(...) ATTRIBUTE(class, __VA_ARGS__)
#define STRUCT(...) ATTRIBUTE(struct, __VA_ARGS__)
#define FIELD(...) ATTRIBUTE(field, __VA_ARGS__)
#define FUNCTION(...) ATTRIBUTE(function, __VA_ARGS__)

enum class ENUM() my_enum
{
    name1 ATTRIBUTE(display = "Name1"),
    name2 ATTRIBUTE(display = "Name2"),
};

struct STRUCT(json) my_struct
{
    FIELD(json = "Integer")
    int i = 0;

    FIELD(json = "String")
    const char* s = nullptr;
};

struct CLASS(inject) my_class
{
    FIELD(inject)
    my_interface* impl = nullptr;

    FUNCTION(script)
    void compute(
        ATTRIBUTE(in) int i,
        ATTRIBUTE(out) float& f);
};
```

## Parser Arguments

All parser arguments are forwarded directly to `libclang`. You can refer
to [this page](https://clang.llvm.org/docs/ClangCommandLineReference.html) for a list of arguments that can be passed
to the parser. Here is an example of most commonly used `libclang` arguments:

```shell
--cpp:-std=c++20
--cpp:-Iinclude
--cpp:-Ithirdparty/include
--cpp:-DSPORE_CODEGEN
--cpp:-DDEBUG
```

## Parser Conditions

### C++ Attributes

Conditions on [attributes](#clang-annotate-attributes) can be configured to control code generation:

```yaml
stages:
  - # ...
    steps:
      - #...
        condition:
          type: attribute
          value:
            generated: true
```

With the given condition, only `C++` input files that contain a symbol with the `generated` attribute would execute the
given step:

```c++
// Matches condition
struct [[clang::annotate("generated")]] matching_data {};

// Does not match condition
struct non_matching_data {};
```

Conditions can contain multiple, nested properties, if you need to match multiple attributes:

```yaml
stages:
  - # ...
    steps:
      - #...
        condition:
          type: attribute
          value:
            generated: true
            override: false
            data:
              value: 42
```

## Implementation Guidelines

### Preventing Circular Dependencies

Since there's a circular dependency between the code and its generated symbols, it's important to prevent generated
code from being included during generation. A good pattern to achieve this is to define a macro during generation to
skip inclusions of generated files.

```c++
#ifndef SPORE_CODEGEN
#  include "project/header.generated.inl"
#endif
```

If you're using the CMake integration, then the `SPORE_CODEGEN` macro will be defined for you by default. If you're not,
you can simply pass this argument directly to the parser: `--cpp:-DSPORE_CODEGEN`. To simplify
include of generated files, you can define this macro somewhere in your project:

```c++
#ifndef SPORE_CODEGEN
#  define GENERATED(File) "project/codegen.hpp"
#else
#  define GENERATED(File) File
#endif

#include GENERATED("project/header.generated.hpp")
```

Also, it's usually a good idea to provide a special file that will be included in place of generated files during
generation. This file should define dummy symbols to prevent warnings in `libclang`. For example, if your configuration
specializes the type `type_traits` for your types, then the file `project/codegen.hpp` could look something like this:

```c++
#pragma once

#ifndef SPORE_CODEGEN
#  error "project/codegen.hpp can only be included during code generation."
#endif 

template <typename value_t>
struct type_traits<value_t>
{
};
```

In this example, we specialize `type_traits` for all types during generation, because `type_traits` is generated, and
thus unavailable during generation. Providing this file helps prevent AST issues with `libclang`, like generation errors
or simply unnamed symbols appearing in the AST.

### Preventing Usage Before Declarations

It is recommended to generate declarations and definitions in different files. It prevents any issues where the
generated code, required for compiling the definitions, is not yet available.

For example, this is a bad pattern that might cause issues eventually:

```c++
// header.hpp

#pragma once

#include "other_header.hpp"

namespace foo
{
    struct bar {};
}

#include GENERATED("header.generated.inl")
```

If `foo::bar` contains any definitions that depends on the content of `header.generated.inl`, then you might end up with
hidden or nasty issues at some point.

You should use this pattern to prevent these issues:

```c++
// header.hpp

#pragma once

#include "other_header.hpp"

#include GENERATED("header.generated.hpp")

namespace foo
{
    struct bar {};
}

#include GENERATED("header.generated.inl")
```

In this example, `header.generated.hpp` should contains forward declaration of `foo::bar` and declarations of all
generated symbols. `header.generated.hpp` should be included after all other includes, and before any declarations.
`header.generated.inl` should contains definitions of all declared symbols and should be included at the end of the
file.

### Useful Macros

Here are some useful macro to ease the integration.

```c++
#pragma once

#ifdef SPORE_CODEGEN
#   define ATTRIBUTE(...) [[clang::annotate(#__VA_ARGS__)]]
#   define GENERATED(File) "project/codegen.hpp"
#else
#   define ATTRIBUTE(...)
#   define GENERATED(File) File
#endif
```
