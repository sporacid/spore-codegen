# Spore Codegen

Spore codegen is a powerful code generation application that uses AST parsing and text templating to provide
build-system agnostic code generation for C++.

It was originally made to power a game engine in order to generate reflection, JSON serialization and shader bindings
for the engine types.

For an example how to integrate `spore-codegen` in your project, you can look
at [this repository](https://github.com/sporacid/spore-codegen-example).

## Requirements

### LLVM

LLVM is required for libclang. Although we used to depend on it through vcpkg, it takes too much time building, so we
expect LLVM to be already installed.

#### Windows

You can find binary distributions of LLVM on their [github](https://github.com/llvm/llvm-project/releases). Afterward,
you need to set the `LLVM_DIR` environment variable to the root of the LLVM install directory.

#### Ubuntu

```shell
sudo apt-get -qq update
sudo apt-get install llvm llvm-dev
```

### Vcpkg

Vcpkg is required to fetch the required dependencies. To allow your project to integrate with `spore-codegen` through
CMake and vcpkg, it's important to set `VCPKG_KEEP_ENV_VARS` environment variable to include `LLVM_DIR` to so that the
variable can be forwarded correctly. If you're not using the `LLVM_DIR` variable, this is not required.

#### Windows

```cmd
set VCPKG_DEFAULT_TRIPLET="x64-windows"
git clone https://github.com/microsoft/vcpkg
cd vcpkg
./bootstrap-vcpkg.bat
set VCPKG_ROOT="%cd%"
set VCPKG_KEEP_ENV_VARS="LLVM_DIR"
```

#### Ubuntu

```shell
export VCPKG_DEFAULT_TRIPLET="x64-linux"
git clone https://github.com/microsoft/vcpkg
cd vcpkg
sudo sh bootstrap-vcpkg.sh
export VCPKG_ROOT=$(cwd)
export VCPKG_KEEP_ENV_VARS="LLVM_DIR"
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
| Template directories | `-T`       | `--template-paths` | Empty             | List of directories in which to search for templates in case the template is not found in the command's working directory.                              |
| Force generate       | `-F`       | `--force`          | `false`           | Skip cache and force generate all input files.                                                                                                          |
| Debug mode           | `-g`       | `--debug`          | `false`           | Enable `debug.json` file with objects for each templates.                                                                                               |
| Verbose mode         | `-V`       | `--verbose`        | `false`           | Enable verbose output.                                                                                                                                  |
| Additional arguments | N/A        | `--`               | Empty             | List of additional arguments to pass verbatim to `libclang` (e.g. `-- -v -stdlib=libc++`).                                                              |

For a minimally working setup, you need at least a configuration file. By default, the application will look for a
the `codegen.json` file at the root of your project.

```json
{
  "version": 1,
  "stages": [
    {
      "name": "headers",
      "directory": "include",
      "files": "**/*.hpp",
      "steps": [
        {
          "name": "generated",
          "directory": "include",
          "templates": [
            "templates/generated.hpp.inja"
          ],
          "condition": {
            "type": "attribute",
            "value": {
              "generated": true
            }
          }
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
  "cache.json"
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
  REFORMAT
  "clang-format -i"
  ADDITIONAL_ARGS
  "-v"
  "-isystem/system/include"
  FORCE
  DEBUG
  VERBOSE
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

#### Inja Template Functions

Some additional functions have been added to the `inja` templating engine to simplify code generation.

| Function                          | Example                                                | Description                                                                              |
|-----------------------------------|--------------------------------------------------------|------------------------------------------------------------------------------------------|
| `this`                            | `{{ this }}`                                           | Access the current object.                                                               |
| `include(string, object)`         | `{{ include("class.inja", class) }}`                   | Include a template with the given object.                                                |
| `json(any)`                       | `{{ json(this) }}`                                     | Dump the given value to JSON with an indent of 2.                                        |
| `json(any, number)`               | `{{ json(this, 4) }}`                                  | Dump the given value to JSON with the given indent.                                      |
| `truthy(any)`                     | `{% if truthy(class.attributes.json) %}{% endif %}`    | Evaluates whether a given value is truthy.                                               |
| `truthy(any, string)`             | `{% if truthy(class.attributes, "json") %}{% endif %}` | Evaluates whether a given value at the given path is truthy.                             |
| `contains(string, string)`        | `{{ contains(class.name, "_test") }}`                  | Checks whether a string contains a given substring.                                      |
| `starts_with(string, string)`     | `{{ starts_with(class.name, "prefix_") }}`             | Checks whether a string starts with the given substring.                                 |
| `ends_with(string, string)`       | `{{ ends_with(class.name, "_suffix") }}`               | Checks whether a string ends with the given substring.                                   |
| `trim_start(string)`              | `{{ trim_start(class.name) }}`                         | Remove any leading whitespaces from the given string.                                    |
| `trim_start(string, string)`      | `{{ trim_start(class.name, "_") }}`                    | Remove any leading characters from the given string.                                     |
| `trim_end(string)`                | `{{ trim_end(class.name) }}`                           | Remove any trailing whitespaces from the given string.                                   |
| `trim_end(string, string)`        | `{{ trim_end(class.name, "_") }}`                      | Remove any trailing characters from the given string.                                    |
| `trim(string)`                    | `{{ trim(class.name) }}`                               | Remove any leading and trailing whitespaces from the given string.                       |
| `trim(string, string)`            | `{{ trim(class.name, "_") }}`                          | Remove any leading and  trailing characters from the given string.                       |
| `replace(string, string, string)` | `{{ replace(class.name, "_t", "_test") }}`             | Replaces a given substring within string with another given substring.                   |
| `cpp_name(string)`                | `{{ cpp_name(path) }}`                                 | Replace any invalid C++ character for an underscore (e.g. `/some-name` -> `_path_name`). |
| `fs.absolute(string)`             | `{{ fs.absolute(path) }}`                              | Get the absolute path of the given path.                                                 |
| `fs.directory(string)`            | `{{ fs.directory(path) }}`                             | Get the directory path of the given path.                                                |
| `fs.filename(string)`             | `{{ fs.filename(path) }}`                              | Get the file name of the given path.                                                     |
| `fs.extension(string)`            | `{{ fs.extension(path) }}`                             | Get the file extension of the given path.                                                |

### JSON Context

The JSON context given to any text template has the following format. For more information on the format of these
object,
you can have a look at [AST objects](https://github.com/sporacid/spore-codegen/tree/main/include/spore/codegen/ast) and
[codegen data](https://github.com/sporacid/spore-codegen/blob/main/include/spore/codegen/codegen_data.hpp).

```json
{
  "$": {
    "stage": {
      /* Current stage data */
    },
    "step": {
      /* Current step data */
    },
    "template": {
      /* Current template data */
    },
    "file": {
      /* Current file data */
    },
    "output": {
      /* Current output data */
    },
    "user_data": {
      /* User data from command line */
    }
  },
  "id": 1,
  "path": "file.hpp",
  "classes": [
    /* All classes found within file.hpp */
  ],
  "enums": [
    /* All enums found within file.hpp */
  ],
  "functions": [
    /* All free functions found within file.hpp */
  ]
}
```

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