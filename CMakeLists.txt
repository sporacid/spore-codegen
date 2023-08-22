cmake_minimum_required(VERSION 3.15)

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0079 NEW)

option(SPORE_BUILD_EXAMPLES "Whether to build examples" ON)
option(SPORE_BUILD_TESTS "Whether to build tests" ON)

set(CMAKE_CXX_STANDARD 17)

project(spore-codegen)

find_package(spore-cmake CONFIG)

if (SPORE_CMAKE_FOUND)
  list(APPEND CMAKE_MODULE_PATH ${SPORE_CMAKE_SCRIPTS})
  if (NOT SPORE_VERSION)
    include(SporeVersion)
    spore_get_version(SPORE_VERSION)
  endif ()
endif ()

if (SPORE_VERSION)
  set(CMAKE_PROJECT_VERSION ${SPORE_VERSION} CACHE STATIC "" FORCE)
endif ()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

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

add_subdirectory(src)

if (SPORE_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif ()

if (SPORE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif ()

