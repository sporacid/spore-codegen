cmake_minimum_required(VERSION 3.15)

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0079 NEW)

option(WITH_EXAMPLES "Whether to include examples" ON)
option(WITH_TESTS "Whether to include tests" ON)
option(WITH_COMPILE_COMMANDS "Whether to generate a compilation database" OFF)

set(CMAKE_CXX_STANDARD 17)

if (WITH_COMPILE_COMMANDS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

list(
  APPEND CMAKE_MODULE_PATH
   ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

if (NOT SPORE_VERSION)
  include(spore_version)
  spore_get_version(SPORE_VERSION)
endif ()

project(spore.codegen VERSION ${SPORE_VERSION})

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/spore/codegen/codegen_version.hpp.in
  ${CMAKE_CURRENT_SOURCE_DIR}/include/spore/codegen/codegen_version.hpp
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(
    SPDLOG_ACTIVE_LEVEL=0
  )
  add_compile_options(
    "$<$<CXX_COMPILER_ID:MSVC>:/Od>"
    "$<$<CXX_COMPILER_ID:GNU>:-g3>"
    "$<$<CXX_COMPILER_ID:GNU>:-O0>"
    "$<$<CXX_COMPILER_ID:Clang>:-g>"
    "$<$<CXX_COMPILER_ID:Clang>:-O0>"
  )
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_definitions(
    SPDLOG_ACTIVE_LEVEL=1
  )
  add_compile_options(
    "$<$<CXX_COMPILER_ID:MSVC>:/Ox>"
    "$<$<CXX_COMPILER_ID:GNU>:-O3>"
    "$<$<CXX_COMPILER_ID:Clang>:-O3>"
  )
endif ()

include(FetchContent_argparse)
include(FetchContent_catch2)
include(FetchContent_cppast)
include(FetchContent_fmt)
include(FetchContent_glob)
include(FetchContent_nlohmann_json)
include(FetchContent_picosha2)
include(FetchContent_inja)
include(FetchContent_spdlog)

add_subdirectory(src)

if (WITH_TESTS)
  add_subdirectory(tests)
endif ()

if (WITH_EXAMPLES)
  add_subdirectory(examples)
endif ()

if (WITH_COMPILE_COMMANDS)
  set(COPY_COMPILE_COMMANDS_FILE "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")

  if(EXISTS "${COPY_COMPILE_COMMANDS_FILE}")
    file(
      COPY "${COPY_COMPILE_COMMANDS_FILE}"
      DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
endif()

