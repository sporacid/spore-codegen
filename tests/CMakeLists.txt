set(
  SPORE_CODEGEN_TESTS_TARGET "${PROJECT_NAME}.tests"
  CACHE STRING "Target for the codegen unit tests"
)

file(
  GLOB_RECURSE SPORE_CODEGEN_TESTS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/**.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/**.cpp
)

add_executable(
  ${SPORE_CODEGEN_TESTS_TARGET}
  ${SPORE_CODEGEN_TESTS_FILES}
)

set_target_properties(
  ${SPORE_CODEGEN_TESTS_TARGET}
  PROPERTIES
    LINKER_LANGUAGE CXX
)

target_link_libraries(
  ${SPORE_CODEGEN_TESTS_TARGET}
  PRIVATE
    ${SPORE_CODEGEN_LIBRARY_TARGET}
    Catch2::Catch2WithMain
)

list(
  APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_BINARY_DIR}/content/catch2/extras
)

include(CTest)
include(Catch)

enable_testing()

catch_discover_tests(${SPORE_CODEGEN_TESTS_TARGET})