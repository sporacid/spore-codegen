set(
  SPORE_CODEGEN_EXAMPLES_JSON_TARGET "${PROJECT_NAME}.examples.json.bin"
  CACHE STRING "Target for the JSON serialziation example"
)

include(spore_codegen)

file(
  GLOB_RECURSE SPORE_CODEGEN_EXAMPLES_JSON_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/**.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp
)

add_executable(
  ${SPORE_CODEGEN_EXAMPLES_JSON_TARGET}
  ${SPORE_CODEGEN_EXAMPLES_JSON_FILES}
)

set_target_properties(
  ${SPORE_CODEGEN_EXAMPLES_JSON_TARGET}
  PROPERTIES
  LINKER_LANGUAGE CXX
)

target_include_directories(
  ${SPORE_CODEGEN_EXAMPLES_JSON_TARGET}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/.codegen/include
)

target_link_libraries(
  ${SPORE_CODEGEN_EXAMPLES_JSON_TARGET}
  PRIVATE
    nlohmann_json
)

spore_codegen(
  ${SPORE_CODEGEN_EXAMPLES_JSON_TARGET}
  DUMP_AST "${CMAKE_CURRENT_SOURCE_DIR}/.dumps"
)
