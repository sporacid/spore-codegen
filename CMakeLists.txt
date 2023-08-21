cmake_minimum_required(VERSION 3.15)

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0079 NEW)

option(WITH_EXAMPLES "Whether to include examples" ON)
option(WITH_TESTS "Whether to include tests" OFF)
option(WITH_COMPILE_COMMANDS "Whether to generate a compilation database" OFF)

set(CMAKE_CXX_STANDARD 17)

if (WITH_COMPILE_COMMANDS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif ()

project(spore-codegen)

if (NOT SPORE_VERSION)
  find_file(SPORE_SEMVER_FILE "spore_semver.cmake" PATH_SUFFIXES "share/spore")
  include(${SPORE_SEMVER_FILE})
  spore_semver(SPORE_VERSION)
endif ()

set(CMAKE_PROJECT_VERSION ${SPORE_VERSION} CACHE STATIC "" FORCE)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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

if (WITH_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif ()

if (WITH_EXAMPLES)
  add_subdirectory(examples)
endif ()

if (WITH_COMPILE_COMMANDS)
  set(COPY_COMPILE_COMMANDS_FILE "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")

  if (EXISTS "${COPY_COMPILE_COMMANDS_FILE}")
    file(
      COPY "${COPY_COMPILE_COMMANDS_FILE}"
      DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}")
  endif ()
endif ()

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/SporeCodegen.cmake
  DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake/spore
)