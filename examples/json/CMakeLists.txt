set(
  SPORE_CODEGEN_EXAMPLES_JSON_TARGET "${PROJECT_NAME}-examples-json-bin"
  CACHE STRING "Target for the JSON serialziation example"
)

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

find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(
  ${SPORE_CODEGEN_EXAMPLES_JSON_TARGET}
  PRIVATE
    nlohmann_json::nlohmann_json
)

include(SporeCodegen)

spore_codegen(
  ${SPORE_CODEGEN_EXAMPLES_JSON_TARGET}
  DEBUG
)