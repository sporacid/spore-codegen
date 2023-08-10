# Spore Codegen

Spore codegen is a powerful code generation application that uses AST parsing and text templating to provide
build-system agnostic code generation for C++.

It was originally made to power a game engine in order to generate reflection, JSON serialization and shader bindings
for the engine types.

For an example how to integrate `spore-codegen` in your project, you can look
at [this repository](https://github.com/sporacid/spore-codegen-example).

## Requirements

### vcpkg

`vcpkg` is required to fetch the required dependencies.

#### Windows

```cmd
set VCPKG_DEFAULT_TRIPLET="x64-windows"
git clone https://github.com/microsoft/vcpkg
cd vcpkg
./bootstrap-vcpkg.bat
set VCPKG_ROOT="%cd%"
```

#### Ubuntu

```shell
export VCPKG_DEFAULT_TRIPLET="x64-linux"
git clone https://github.com/microsoft/vcpkg
cd vcpkg
sudo sh bootstrap-vcpkg.sh
export VCPKG_ROOT=$(cwd)
```

## How to build

### Windows

```cmd
cmake -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -B.cmake -G 'Ninja'
cmake --build .cmake --config Release
```

### Ubuntu

```shell
cmake -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake -B.cmake -G 'Ninja'
cmake --build .cmake --config Release
```

## How to use

Spore codegen is a command line application that can be added to any build pipeline.

| Argument             | Short Name | Long Name          | Default Value     | Description                                                                                                                                             |
|----------------------|------------|--------------------|-------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------|
| Output directory     | `-o`       | `--output`         | `.codegen`        | Output directory where to write generated files.                                                                                                        |
| Configuration file   | `-c`       | `--config`         | `codegen.json`    | Configuration file to use. Contains all codegen steps to execute, which files to process and with which templates.                                      |
| Cache file           | `-C`       | `--cache`          | `cache.json`      | Cache file to use. Hash of output and template files will be written to this file to skip future generation if unnecessary.                             |
| Include directories  | `-I`       | `--includes`       | Empty             | List of include directories to be passed to `libclang`. Used for AST parsing.                                                                           |
| Macro definitions    | `-D`       | `--definitions`    | Empty             | Macro definitions to be passed to `libclang`. Used for AST preprocessing. Can either be passed as `DEFINITION` or `DEFINITION=VALUE`.                   |
| User data            | `-d`       | `--user-data`      | Empty             | Additional user data to be passed to the rendering stage. Can be passed as `key=value` and will be accessible through the `user_data` JSON property.    |
| C++ standard         | `-s`       | `--cpp-standard`   | `c++14`           | C++ standard to be passed to `libclang`. Can be prefixed with `c++` or not (e.g. `c++17` or `17`).                                                      |
| Reformat command     | `-r`       | `--reformat`       | `clang-format -i` | Set the command to execute to reformat output files. The command will be supplied a single argument which will be the relative path to the output file. |
| Force generate       | `-F`       | `--force`          | `false`           | Skip cache and force generate all input files.                                                                                                          |
| Debug mode           | `-g`       | `--debug`          | `false`           | Enable debug logs and dump `debug.json` file in output directory.                                                                                       |
| Template directories | `-T`       | `--template-paths` | Empty             | List of directories in which to search for templates in case the template is not found in the command's WORKING_DIRECTORY                               |

For a minimally working setup, you need at least a configuration file. By default, the application will look for a
the `codegen.json` file at the root of your project.

```json{
  "version": 1,
  "stages": [
    {
      "name": "headers",
      "directory": "include",
      "files": "**/*.hpp",
      "steps": [
        {
          "name": "json",
          "directory": "include",
          "templates": [
            "templates/generated.hpp.inja"
          ]
        }
      ]
    }
  ]
}
```

The base name of the template file and the directory and base name of the input file will be used to generate the output
file. Given:

- An input file `include/project/header.hpp`
- A template file `templates/generated.inl.inja`
- An output directory `.codegen`

Then the output file would be `.codegen/include/project/header.generated.inl`. You can include this file
in `include/project/header.hpp`
with the `SPORE_CODEGEN` guard to prevent circular inclusion during generation.

```cpp
#ifndef SPORE_CODEGEN
#include "project/header.generated.inl"
#endif
```

To simplify usage, you can define this macro somewhere in your project.

```cpp
#ifndef SPORE_CODEGEN
// dummy file to include only when in codegen
#define GENERATED(File) "project/codegen.hpp"
#else
#define GENERATED(File) File
#endif

#include GENERATED("project/header.generated.inl")
```

### CMake Integration

Scripts in `cmake/spore.cmake` can be included to have access to automatic integration to your `cmake` project. The
function `spore_codegen` will automatically deduce definitions and include directories from your target and make the
codegen target a dependency to your target. You can provide the same arguments as the command line to the function, and
they will be forwarded to the executable.

```cmake
# With all arguments
spore_codegen(
  "target"
  OUTPUT
  ".codegen"
  CONFIG
  "codegen.json"
  CACHE
  "codegen.cache.json"
  CXX_STANDARD
  "c++17"
  DEFINITIONS
  "DEFINITION1=value1"
  "DEFINITION2=value2"
  INCLUDES
  "additional/include1"
  "additional/include2"
  USER_DATA
  "key1=value1"
  "key2=value2"
  FORCE
  REFORMAT
  DEBUG
)

# With all defaults
spore_codegen(
  "target"
)
```

## Templates

The application uses text templates to render the AST into actual code files.

### Inja Templates

https://github.com/pantor/inja

Template files with the `.inja` extension will be handled by the inja text templating engine.

Here is a trivial example to give an idea of that an `inja` template looks like.

```jinja
#pragma once

{% for class in classes %}
    // class {{ class.name }}
{% endfor %}

{% for enum in enums %}
    // enum {{ enum.name }}
{% endfor %}

{% for function in functions %}
    // function {{ function.name }}
{% endfor %}
```

Some additional functions have been added to the `inja` templating engine to simplify code generation.

| Function                          | Example                                                              | Description                                                                              |
|-----------------------------------|----------------------------------------------------------------------|------------------------------------------------------------------------------------------|
| `this`                            | `{{ this }}`                                                         | Access the current object.                                                               |
| `truthy(any)`                     | `{% if truthy(class.attributes.json) %}{% endif %}`                  | Evaluates whether a given value is truthy.                                               |
| `truthy(any, string)`             | `{% if truthy(class.attributes, "json.values.ignore") %}{% endif %}` | Evaluates whether a given value with an hypothetical property path is truthy.            |
| `json(any)`                       | `{{ json(this) }}`                                                   | Dump the given value to JSON with an indent of 2.                                        |
| `json(any, number)`               | `{{ json(this, 4) }}`                                                | Dump the given value to JSON with the given indent.                                      |
| `include(string, object)`         | `{{ include("class.inja", class) }}`                                 | Include a template with the given object.                                                |
| `contains(string, string)`        | `{{ contains(class.name, "_test") }}`                                | Checks whether a string contains a given substring.                                      |
| `replace(string, string, string)` | `{{ replace(class.name, "_t", "_test") }}`                           | Replaces a given substring within string with another given substring.                   |
| `cpp_name(string)`                | `{{ cpp_name(path) }}`                                               | Replace any invalid C++ character for an underscore (e.g. `/some-name` -> `_path_name`). |
| `abs_path(string)`                | `{{ abs_path(path) }}`                                               | Transform for the given path to an absolute path.                                        |

### Additional Template Engines

For now, only `inja` templates are supported, but everything is in place to support additional engine in the future,
such as mustache.

## Attributes

Attributes are parsed using `clang` annotate attribute. The content of the attribute is parsed into a JSON object, from
a unique syntax to improve usability.

| Scenario         | Syntax                                                                     | JSON result                                            | Notes                                                                                        |
|------------------|----------------------------------------------------------------------------|--------------------------------------------------------|----------------------------------------------------------------------------------------------|
| Single Flag      | `flag` or `flag = false`                                                   | `{"flag": true}` or `{"flag": false}`                  | By default, a flag is set to true.                                                           |
| Single Value     | `value = 1` or `value = 1.0` or `value = "1"`                              | `{"value": 1}` or `{"value": 1.0}` or `{"value": "1"}` | Values can be `int`, `float` or `string`. Technically, flags are also values of type `bool`. |
| Single Object    | `object = (flag, value = 1)`                                               | `{"object": {"flag": true, "value": 1}}`               | Objects are denoted with parenthesis.                                                        |
| Overridden Value | `value = 1, value = 2`                                                     | `{"value": 2}`                                         | Subsequent values with the same keys override the previous value.                            |
| Merged Object    | `object = (flag, value = 1), object = (flag = false, value = 2, new = 42)` | `{"object": {"flag": false, "value": 2, "new": 42}}`   | Subsequent objects with the same keys are merged with the previous value.                    |
| Nested Objects   | `object = (nested = (flag))`                                               | `{"object": {"nested": {"flag": true}}}`               | Objects are recursive and can be nested without any limits.                                  |

### Examples

Here is a simple example using the `clang::annotate` attribute on a struct.

```cpp
struct [[clang::annotate("flag, value = 1")]] data
{
};
```

Here is a more complex example with macro definitions to simplify usage.

```cpp
#ifdef SPORE_CODEGEN
#define ATTRIBUTE(...) [[clang::annotate(#__VA_ARGS__)]]
#define GENERATED(File) "project/codegen.hpp"
#else
#define ATTRIBUTE(...)
#define GENERATED(File) File
#endif

#define ENUM(...) ATTRIBUTE(enum, __VA_ARGS__)
#define CLASS(...) ATTRIBUTE(class, __VA_ARGS__)
#define STRUCT(...) ATTRIBUTE(struct, __VA_ARGS__)
#define FIELD(...) ATTRIBUTE(field, __VA_ARGS__)
#define FUNCTION(...) ATTRIBUTE(function, __VA_ARGS__)

#include GENERATED("project/file.generated.hpp")

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
    class my_interface* impl = nullptr;

    FUNCTION(script)
    void compute(
        ATTRIBUTE(in) int i,
        ATTRIBUTE(out) float& f);
};

#include GENERATED("project/file.generated.inl")
```