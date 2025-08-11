set(TARGET_NAME ${PROJECT_NAME}-examples-enum)

list(
  APPEND TARGET_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

add_executable(
  ${TARGET_NAME}
  ${TARGET_FILES}
)

target_include_directories(
  ${TARGET_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/.codegen/include
)

include(SporeCodegen)

spore_codegen(
  ${TARGET_NAME}
  CACHE .codegen/cache.yml
  REFORMAT
  DEBUG
)

set_target_properties(
  ${TARGET_NAME} PROPERTIES
  ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_SOURCE_DIR}/.codegen
)