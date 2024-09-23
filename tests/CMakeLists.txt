set(
  SPORE_CODEGEN_TESTS_TARGET "${PROJECT_NAME}-tests"
  CACHE STRING "Target for the codegen unit tests"
)

if (SPORE_WITH_CPP)
  list(APPEND SPORE_CODEGEN_TESTS_FILES t_codegen_parser_cpp.cpp)
endif ()

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

find_package(Catch2 CONFIG REQUIRED)

include(Catch)

catch_discover_tests(${SPORE_CODEGEN_TESTS_TARGET})