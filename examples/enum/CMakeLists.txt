set(
  SPORE_CODEGEN_EXAMPLES_ENUM_BIN "${PROJECT_NAME}.examples.enum.bin"
  CACHE INTERNAL
    SPORE_CODEGEN_EXAMPLES_ENUM_BIN
)

include(spore)

file(
  GLOB_RECURSE SPORE_CODEGEN_EXAMPLES_ENUM_BIN_FILES
    ${CMAKE_SOURCE_DIR}/examples/enum/include/**.hpp
    ${CMAKE_SOURCE_DIR}/examples/enum/src/**.cpp
)

add_executable(
  ${SPORE_CODEGEN_EXAMPLES_ENUM_BIN}
  ${SPORE_CODEGEN_EXAMPLES_ENUM_BIN_FILES}
)

set_target_properties(
  ${SPORE_CODEGEN_EXAMPLES_ENUM_BIN}
  PROPERTIES
  LINKER_LANGUAGE CXX
)

target_include_directories(
  ${SPORE_CODEGEN_EXAMPLES_ENUM_BIN}
  PRIVATE
    ${CMAKE_SOURCE_DIR}/examples/enum/include
    ${CMAKE_SOURCE_DIR}/examples/enum/.codegen/include
)

spore_codegen(
  ${SPORE_CODEGEN_EXAMPLES_ENUM_BIN}
  REFORMAT
)