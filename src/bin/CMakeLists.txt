set(TARGET_NAME ${PROJECT_NAME}-bin)

set(SPORE_CODEGEN_BIN ${TARGET_NAME} CACHE STRING "Target for the codegen binary")

list(APPEND TARGET_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

find_package(argparse CONFIG REQUIRED)

add_executable(${TARGET_NAME} ${TARGET_FILES})

target_link_libraries(
  ${TARGET_NAME} PRIVATE
  ${SPORE_CODEGEN_LIB}
  argparse::argparse
)

set_target_properties(
  ${TARGET_NAME} PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}
)