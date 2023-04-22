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

target_include_directories(
  ${SPORE_CODEGEN_LIBRARY_TARGET}
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_BINARY_DIR}/content/chaiscript/include
)

target_link_libraries(
  ${SPORE_CODEGEN_LIBRARY_TARGET}
  INTERFACE
    ${CHAISCRIPT_LIBS}
    cppast
    fmt
    Glob
    inja
    nlohmann_json
    picosha2
    spdlog
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/**.hpp
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
    argparse
)
