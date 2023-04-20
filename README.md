# Spore Codegen

Spore codegen is a powerful code generation application that uses AST parsing and text templating to provide
build-system agnostic code generation for C++.

It was originally made to power a game engine in order to generate reflection, JSON serialization and shader bindings
for the engine types.

For an example how to integrate `spore-codegen` in your project, you can look
at [this repository](https://github.com/sporacid/spore-codegen-example).

## Requirements

### LLVM

`LLVM` and `libclang` are required to parse the C++ AST.

#### Windows

```cmd
set VCPKG_DEFAULT_TRIPLET="x64-windows"
git clone https://github.com/microsoft/vcpkg
cd vcpkg
./bootstrap-vcpkg.bat
./vcpkg.exe install llvm
set LLVM_CONFIG_BINARY="%cd%/installed/%VCPKG_DEFAULT_TRIPLET%/tools/llvm/llvm-config.exe"
```

#### Ubuntu

```sh
sudo apt-get -qq update
sudo apt-get install -y llvm clang clang-format libclang-dev
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
| Features             | `-f`       | `--features`       | Empty             | Feature flags to be passed to `libclang`. Used for AST preprocessing. **Disabled for now because of some integration issues.**                          |
| User data            | `-d`       | `--user-data`      | Empty             | Additional user data to be passed to the rendering stage. Can be passed as `key=value` and will be accessible through the `user_data` JSON property.    |
| C++ standard         | `-s`       | `--cpp-standard`   | `c++14`           | C++ standard to be passed to `libclang`. Can be prefixed with `c++` or not (e.g. `c++17` or `17`).                                                      |
| Reformat command     | `-r`       | `--reformat`       | `clang-format -i` | Set the command to execute to reformat output files. The command will be supplied a single argument which will be the relative path to the output file. |
| Force generate       | `-F`       | `--force`          | `false`           | Skip cache and force generate all input files.                                                                                                          |
| Debug mode           | `-g`       | `--debug`          | `false`           | Enable debug logs.                                                                                                                                      |
| Sequential mode      | `-S`       | `--sequential`     | `false`           | Enable sequential processing instead of the default parallel processing.                                                                                |
| Dump AST             |            | `--dump-ast`       | Empty             | Output directory where to write JSON dumps of the AST.                                                                                                  |
| Template directories | `-T`       | `--template-paths` | Empty             | List of directories in which to search for templates in case the template is not found in the command's WORKING_DIRECTORY                               |

For a minimally working setup, you need at least a configuration file. By default, the application will look for a
the `codegen.json` file at the root of your project.

```json
{
  "steps": [
    {
      "name": "headers",
      "input": "include/**/*.hpp",
      "templates": [
        "templates/generated.hpp.inja"
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

Then the output file will be `.codegen/include/project/header.generated.inl`. You can include this file at the end of
the `include/project/header.hpp` file with the `SPORE_CODEGEN` guard to prevent circular inclusion during generation.

```cpp
#ifndef SPORE_CODEGEN
#include "project/header.generated.inl"
#endif
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
  SEQUENTIAL
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

| Function              | Example                                                              | Description                                                                              |
|-----------------------|----------------------------------------------------------------------|------------------------------------------------------------------------------------------|
| `truthy(any)`         | `{% if truthy(class.attributes.json) %}{% endif %}`                  | Evaluates whether a given value is truthy.                                               |
| `truthy(any, string)` | `{% if truthy(class.attributes, "json.values.ignore") %}{% endif %}` | Evaluates whether a given value with an hypothetical property path is truthy.            |
| `cpp_name(string)`    | `{{ cpp_name(path) }}`                                               | Replace any invalid C++ character for an underscore (e.g. `/some-name` -> `_path_name`). |

### Additional Template Engines

For now, only `inja` templates are supported, but everything is in place to support additional engine in the future,
such as mustache.
