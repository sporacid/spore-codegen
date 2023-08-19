set(
  SPORE_CODEGEN_BINARY_TARGET "${PROJECT_NAME}.bin"
  CACHE STRING "Target for the main codegen application"
)

set(
  SPORE_CODEGEN_LIBRARY_TARGET "${PROJECT_NAME}.lib"
  CACHE STRING "Target for the main codegen library"
)

set(
  SPORE_CODEGEN_BINARY_NAME "spore-codegen"
  CACHE STRING "Output name for the main codegen application"
)

add_library(
  ${SPORE_CODEGEN_LIBRARY_TARGET}
  INTERFACE
)

find_path(INJA_INCLUDE_DIRS "inja.hpp")
find_path(PICOSHA2_INCLUDE_DIRS "picosha2.h")

target_include_directories(
  ${SPORE_CODEGEN_LIBRARY_TARGET}
  INTERFACE
    ${CMAKE_SOURCE_DIR}/include
    ${PICOSHA2_INCLUDE_DIRS}
    ${INJA_INCLUDE_DIRS}
)

find_package(LLVM CONFIG REQUIRED)
find_package(argparse CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(inja CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

include(FetchContent_cppast)
include(FetchContent_glob)

target_link_libraries(
  ${SPORE_CODEGEN_LIBRARY_TARGET}
  INTERFACE
    Glob
    cppast
    fmt::fmt
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
    CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  find_package(TBB REQUIRED)
  target_link_libraries(
    ${SPORE_CODEGEN_LIBRARY_TARGET}
    INTERFACE
      TBB::tbb
  )
endif ()

file(
  GLOB_RECURSE SPORE_CODEGEN_FILES
    ${CMAKE_SOURCE_DIR}/include/**.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/**.cpp
)

add_executable(
  ${SPORE_CODEGEN_BINARY_TARGET}
  ${SPORE_CODEGEN_FILES}
)

set_target_properties(
  ${SPORE_CODEGEN_BINARY_TARGET}
  PROPERTIES
    LINKER_LANGUAGE CXX
    OUTPUT_NAME ${SPORE_CODEGEN_BINARY_NAME}
)

target_link_libraries(
  ${SPORE_CODEGEN_BINARY_TARGET}
  PRIVATE
    ${SPORE_CODEGEN_LIBRARY_TARGET}
    argparse::argparse
)
