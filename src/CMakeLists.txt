set(
  SPORE_CODEGEN_BIN "${PROJECT_NAME}.bin"
  CACHE INTERNAL
    SPORE_CODEGEN_BIN
)

file(
  GLOB_RECURSE SPORE_CODEGEN_BIN_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/**.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/**.cpp
)

add_executable(
  ${SPORE_CODEGEN_BIN}
  ${SPORE_CODEGEN_BIN_FILES}
)

set_target_properties(
  ${SPORE_CODEGEN_BIN}
  PROPERTIES
    LINKER_LANGUAGE CXX
)

target_include_directories(
  ${SPORE_CODEGEN_BIN}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(
  ${SPORE_CODEGEN_BIN}
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
    ${SPORE_CODEGEN_BIN}
    PRIVATE
      TBB::tbb
  )
endif ()