set(
  SPORE_CODEGEN_EXAMPLES_ENUM_TARGET "${PROJECT_NAME}-examples-enum-bin"
  CACHE STRING "Target for the enum to string example"
)

file(
  GLOB_RECURSE SPORE_CODEGEN_EXAMPLES_ENUM_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/**.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp
)

add_executable(
  ${SPORE_CODEGEN_EXAMPLES_ENUM_TARGET}
  ${SPORE_CODEGEN_EXAMPLES_ENUM_FILES}
)

set_target_properties(
  ${SPORE_CODEGEN_EXAMPLES_ENUM_TARGET}
  PROPERTIES
  LINKER_LANGUAGE CXX
)

target_include_directories(
  ${SPORE_CODEGEN_EXAMPLES_ENUM_TARGET}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/.codegen/include
)

include(SporeCodegen)

spore_codegen(
  ${SPORE_CODEGEN_EXAMPLES_ENUM_TARGET}
  CACHE .codegen/cache.yml
  DEBUG
)