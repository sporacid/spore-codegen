set(
  SPORE_CODEGEN_EXAMPLES_JSON_BIN "${PROJECT_NAME}.examples.json.bin"
  CACHE INTERNAL
    SPORE_CODEGEN_EXAMPLES_JSON_BIN
)

include(spore)

file(
  GLOB_RECURSE SPORE_CODEGEN_EXAMPLES_JSON_BIN_FILES
    ${CMAKE_SOURCE_DIR}/examples/json/include/**.hpp
    ${CMAKE_SOURCE_DIR}/examples/json/src/**.cpp
)

add_executable(
  ${SPORE_CODEGEN_EXAMPLES_JSON_BIN}
  ${SPORE_CODEGEN_EXAMPLES_JSON_BIN_FILES}
)

set_target_properties(
  ${SPORE_CODEGEN_EXAMPLES_JSON_BIN}
  PROPERTIES
  LINKER_LANGUAGE CXX
)

target_include_directories(
  ${SPORE_CODEGEN_EXAMPLES_JSON_BIN}
  PRIVATE
    ${CMAKE_SOURCE_DIR}/examples/json/include
    ${CMAKE_SOURCE_DIR}/examples/json/.codegen/include
)

target_link_libraries(
  ${SPORE_CODEGEN_EXAMPLES_JSON_BIN}
  PRIVATE
    nlohmann_json
)

spore_codegen(
  ${SPORE_CODEGEN_EXAMPLES_JSON_BIN}
  REFORMAT
)