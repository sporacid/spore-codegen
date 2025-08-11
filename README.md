# What is Spore Codegen

`spore-codegen` is a powerful code generation application that uses AST parsing and text templating to provide
build-system agnostic code generation for languages such as `C++` and binary formats such as `SPIR-V`.

It was originally made to power a game engine in order to generate reflection, `JSON` serialization and `SPIR-V`
bindings for the engine types.

# Quick Start

1. Add a `codegen.yml` configuration file to your project:

    ```yaml
    version: 1
    stages:
      - name: headers
        parser: cpp
        directory: include
        files: "**/*.hpp"
        steps:
          - name: headers.generated.inl
            directory: .codegen/include
            templates:
              - templates/generated.inl.inja
    ```

2. Add the `templates/generated.inl.inja` template file to your project:

    ```txt
    #pragma once
    
    {% for class in classes %}
   
        template <>
        struct type_traits<{{ class.full_name }}>
        {
        };
   
    {% endfor %}
    ```

3. Invoke `spore-codegen` executable in your project directory.
4. Voilà! You should have your generated headers in the `.codegen/include`. Don't forget to add the cache file
   `.codegen.yml` and the generated directory to your `.gitignore`.

## More Examples

For more examples, you can refer to the [examples](examples) directory of this project, or you can look
at [this repository](https://github.com/sporacid/spore-codegen-example) for a more complete, standalone example.

## Table of Contents

- [Installation](#installation)
    - [Requirements](#requirements)
    - [Install Steps](#install-steps)
        - [Windows](#windows)
        - [Ubuntu](#ubuntu)
    - [Build Steps](#build-steps)
        - [Windows](#windows-1)
        - [Ubuntu](#ubuntu-1)
- [Usage](#usage)
- [Configuration](#configuration)
    - [Format](#format)
    - [Output Files](#output-files)
- [Parsers](#parsers)
    - [C++](#c)
    - [SPIR-V](#spir-v)
- [Integrations](#integrations)
    - [CMake](#cmake)
    - [Vcpkg](#vcpkg)
- [Templates](#templates)
    - [Inja Templates](#inja-templates)
        - [Inja Template Functions](#inja-template-functions)
    - [JSON Context](#json-context)
    - [Additional Template Engines](#additional-template-engines)

# Installation

## Requirements

- **LLVM**: Required for parsing C++.
- **Vcpkg**: Required to fetch dependencies.
- **CMake**: Required to build the project.

## Install Steps

### Windows

1. Install `LLVM`:
    ```shell
    git clone https://github.com/llvm/llvm-project llvm --branch llvmorg-20.1.8
    cd llvm
    mkdir .build .install
    cmake -S llvm -B .build -G "Ninja" -DCMAKE_BUILD_TYPE="Release" -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_ADDITIONAL_BUILD_TYPES="Debug" -DLLVM_TARGETS_TO_BUILD="host"
    cmake --build .build
    cmake --install .build --prefix .install
    set PATH="%PATH%;%cd%/.install;%cd%/.install/bin"
   ```

2. Clone and set up `Vcpkg`:
   ```shell
   set VCPKG_DEFAULT_TRIPLET="x64-windows"
   git clone https://github.com/microsoft/vcpkg
   cd vcpkg
   ./bootstrap-vcpkg.bat
   set VCPKG_ROOT="%cd%"
   ```

### Ubuntu

1. Install `LLVM`:

   ```shell
    export LLVM_VERSION=20
    wget https://apt.llvm.org/llvm.sh
    chmod u+x llvm.sh
    sudo ./llvm.sh ${LLVM_VERSION} all
    export PATH="$PATH:/usr/lib/llvm-${LLVM_VERSION}"
   ```

2. Clone and set up `Vcpkg`:

   ```shell
   export VCPKG_DEFAULT_TRIPLET="x64-linux"
   git clone https://github.com/microsoft/vcpkg
   cd vcpkg
   sudo sh bootstrap-vcpkg.sh
   export VCPKG_ROOT=$(pwd)
   ```

## Build Steps

`CMake` is used to build the project, with `Vcpkg` toolchain.

### Windows

```shell
cmake -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -B.cmake -G 'Ninja'
cmake --build .cmake --config Release
```

### Ubuntu

```shell
cmake -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake -B.cmake -G 'Ninja'
cmake --build .cmake --config Release
```

## Usage

`spore-codegen` is a command line application that can be added to any build pipeline.

| Argument             | Short | Long                    | Default        | Description                                                                                                                                            |
|----------------------|-------|-------------------------|----------------|--------------------------------------------------------------------------------------------------------------------------------------------------------|
| Configuration file   | `-c`  | `--config`              | `codegen.yml`  | Configuration file to use. Contains all codegen steps to execute, which files to process and with which templates.                                     |
| Cache file           | `-C`  | `--cache`               | `.codegen.yml` | Cache file to use. This file will be used to detect whether parsing and generation is required for any given input files.                              |
| Template directories | `-t`  | `--templates`           | Empty          | List of directories in which to search for templates in case the template is not found in the command's working directory.                             |
| User data            | `-D`  | `--user-data`           | Empty          | Additional user data to be passed to the rendering stage. Can be passed as `key=value` and will be accessible through the `$.user_data` JSON property. |
| Reformat             | `-r`  | `--reformat`            | `false`        | Whether to reformat output files. Will use `.clang-format` configuration file for `cpp` files.                                                         |
| Force generate       | `-f`  | `--force`               | `false`        | Skip cache and force generate all input files.                                                                                                         |
| Debug mode           | `-d`  | `--debug`               | `false`        | Enable debug output.                                                                                                                                   |
| Parser arguments     | N/A   | `--<parser>:<argument>` | Empty          | Additional arguments to pass verbatim to the parser implementation (e.g. `--cpp:-std=c++20 --cpp:-Iproject/include`).                                  |

# Configuration

The configuration file defines stages and steps for the generation. Stages defines the input files and the parser to use
and steps define templates and output files to generate. All outputs from a given stage can be re-used as an input for
the next stage. For example, you could generate `SPIR-V` bindings in the first stage and generate reflection data for
these bindings in the second stage.

## Format

```yaml
version: 1
stages:
  - name: "stage"         # Name of the stage for logging purposes
    parser: "cpp"         # Name of the parser to use (e.g. cpp or spirv)
    directory: "include"  # Input directory for this stage (application will change working directory to this)
    files: "**/*.hpp"     # Glob pattern to find input files
    steps:
      - name: "step"                   # Name of the step for logging purposes
        directory: ".codegen/include"  # Output directory for generated files
        templates: # List of templates to generate for this step
          - "template.inl.inja"
        condition: # Optional condition for this step, evaluated per input file
          type: "attribute"            # Type of the condition
          value: # Optional value to match the condition
            generated: true            # e.g. Matches only C++ files which contain an element with the attribute `generated` set to true 
```

## Output Files

Output file names are automatically generated from the stage input file, the template file and the step output
directory. For example, given:

- An input file `include/project/header.hpp`
- A template file `templates/generated.inl.inja`
- A step output directory `.codegen/include`

The resulting output file would be `.codegen/include/project/header.generated.inl`.

# Parsers

## C++

For details on `C++` parsing and generation, please refer to [this page](docs/ParserCpp.md).

## SPIR-V

For details on `SPIR-V` parsing and generation, please refer to [this page](docs/ParserSpirv.md).

# Integrations

## CMake

Scripts in `cmake/SporeCodegen.cmake` can be included to have access to automatic integration to your `CMake` project.
The function `spore_codegen` will automatically deduce definitions and include directories from your target and make the
codegen target a dependency to your target. You can provide the same arguments as the command line to the function, and
they will be forwarded to the executable.

```cmake
# With all defaults
spore_codegen(target)

# With all arguments
spore_codegen(
  target
  CONFIG codegen.yml
  CACHE cache.yml
  USER_DATA
    key1=value1
    key2=value2
  ADDITIONAL_ARGS
    --cpp:-isystem/system/include
    --cpp:-v
  REFORMAT
  FORCE
  DEBUG
)
```

## Vcpkg

`spore-codegen` is available through the [spore-vcpkg repository](https://github.com/sporacid/spore-vcpkg).

1. In `vcpkg-configuration.json`:

   ```json
   {
     "registries": [
       {
         "kind": "git",
         "repository": "https://github.com/sporacid/spore-vcpkg",
         "baseline": "<baseline>",
         "packages": [
           "spore-codegen"
         ]
       }
     ]
   }
   ```

2. In `vcpkg.json`:

   ```json
   {
     "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
     "name": "project",
     "version": "1.0",
     "builtin-baseline": "<baseline>",
     "dependencies": [
       {
         "name": "spore-codegen",
         "host": true
       }
     ]
   }
   ```

# Templates

The application uses text templates to render AST objects into actual code files.

## Inja Templates

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

### Inja Template Functions

Some additional functions have been added to the `inja` templating engine to simplify code generation.

| Function                          | Example                                                | Description                                                                                            |
|-----------------------------------|--------------------------------------------------------|--------------------------------------------------------------------------------------------------------|
| `this`                            | `{{ this }}`                                           | Access the current object.                                                                             |
| `include(string, object)`         | `{{ include("class.inja", class) }}`                   | Include a template with the given object.                                                              |
| `json(any)`                       | `{{ json(this) }}`                                     | Dump the given value to JSON with an indent of 2.                                                      |
| `json(any, number)`               | `{{ json(this, 4) }}`                                  | Dump the given value to JSON with the given indent.                                                    |
| `flatten(object)`                 | `{{ flatten(this) }}`                                  | Flatten the given object into an object of depth 1.                                                    |
| `flatten(object, string)`         | `{{ flatten(this, ":") }}`                             | Flatten the given object into an object of depth 1 with the given key separator.                       |
| `yaml(any)`                       | `{{ yaml(this) }}`                                     | Dump the given value to YAML with an indent of 2.                                                      |
| `yaml(any, number)`               | `{{ yaml(this, 4) }}`                                  | Dump the given value to YAML with the given indent.                                                    |
| `truthy(any)`                     | `{% if truthy(class.attributes.json) %}{% endif %}`    | Evaluates whether a given value is truthy.                                                             |
| `truthy(any, string)`             | `{% if truthy(class.attributes, "json") %}{% endif %}` | Evaluates whether a given value at the given path is truthy.                                           |
| `contains(string, string)`        | `{{ contains(class.name, "_test") }}`                  | Checks whether a string contains a given substring.                                                    |
| `starts_with(string, string)`     | `{{ starts_with(class.name, "prefix_") }}`             | Checks whether a string starts with the given substring.                                               |
| `ends_with(string, string)`       | `{{ ends_with(class.name, "_suffix") }}`               | Checks whether a string ends with the given substring.                                                 |
| `trim_start(string)`              | `{{ trim_start(class.name) }}`                         | Remove any leading whitespaces from the given string.                                                  |
| `trim_start(string, string)`      | `{{ trim_start(class.name, "_") }}`                    | Remove any leading characters from the given string.                                                   |
| `trim_end(string)`                | `{{ trim_end(class.name) }}`                           | Remove any trailing whitespaces from the given string.                                                 |
| `trim_end(string, string)`        | `{{ trim_end(class.name, "_") }}`                      | Remove any trailing characters from the given string.                                                  |
| `trim(string)`                    | `{{ trim(class.name) }}`                               | Remove any leading and trailing whitespaces from the given string.                                     |
| `trim(string, string)`            | `{{ trim(class.name, "_") }}`                          | Remove any leading and  trailing characters from the given string.                                     |
| `replace(string, string, string)` | `{{ replace(class.name, "_t", "_test") }}`             | Replaces a given substring within string with another given substring.                                 |
| `cpp.name(string)`                | `{{ cpp.name(path) }}`                                 | Replace any invalid C++ character for an underscore (e.g. `/some-name` -> `_path_name`).               |
| `cpp.embed(binary)`               | `{{ cpp.embed(base64) }}`                              | Embed the given binary as hex codes, to be used within a `C` or `C++` byte array.                      |
| `cpp.embed(binary, number)`       | `{{ cpp.embed(base64, 80) }}`                          | Embed the given binary as hex codes with the given width, to be used within a `C` or `C++` byte array. |
| `fs.absolute(string)`             | `{{ fs.absolute(path) }}`                              | Get the absolute path of the given path.                                                               |
| `fs.directory(string)`            | `{{ fs.directory(path) }}`                             | Get the directory path of the given path.                                                              |
| `fs.filename(string)`             | `{{ fs.filename(path) }}`                              | Get the file name of the given path.                                                                   |
| `fs.extension(string)`            | `{{ fs.extension(path) }}`                             | Get the file extension of the given path.                                                              |

## JSON Context

The JSON context given to any text template has the following format. For more information on the format of these
object, you can have a look at [AST objects](include/spore/codegen/ast)
and [codegen data](include/spore/codegen/codegen_data.hpp).

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
  }
  /* AST data */
}
```

## Additional Template Engines

For now, only `inja` templates are supported, but everything is in place to support additional engine in the future,
such as mustache.
