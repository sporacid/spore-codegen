set(
  SPORE_CODEGEN_TARGET "${PROJECT_NAME}.bin"
  CACHE STRING "Target for the main codegen application"
)

set(
  SPORE_CODEGEN_OUTPUT "spore-codegen"
  CACHE STRING "Output name for the main codegen application"
)

file(
  GLOB_RECURSE SPORE_CODEGEN_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/**.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/**.cpp
)

add_executable(
  ${SPORE_CODEGEN_TARGET}
  ${SPORE_CODEGEN_FILES}
)

set_target_properties(
  ${SPORE_CODEGEN_TARGET}
  PROPERTIES
    LINKER_LANGUAGE CXX
    OUTPUT_NAME ${SPORE_CODEGEN_OUTPUT}
)

target_include_directories(
  ${SPORE_CODEGEN_TARGET}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(
  ${SPORE_CODEGEN_TARGET}
  PRIVATE
    argparse
    cppast
    fmt
    Glob
    inja
    nlohmann_json
    spdlog
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  find_package(TBB REQUIRED)

  target_link_libraries(
    ${SPORE_CODEGEN_TARGET}
    PRIVATE
      TBB::tbb
  )
endif ()