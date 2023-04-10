set(SPORE_CODEGEN_TESTS "${PROJECT_NAME}.tests" CACHE INTERNAL SPORE_CODEGEN_TESTS)

file(
  GLOB_RECURSE SPORE_CODEGEN_TESTS_FILES
    ${CMAKE_SOURCE_DIR}/include/**.hpp
    ${CMAKE_SOURCE_DIR}/tests/**.cpp
)

add_executable(
  ${SPORE_CODEGEN_TESTS}
  ${SPORE_CODEGEN_TESTS_FILES}
)

set_target_properties(
  ${SPORE_CODEGEN_TESTS}
  PROPERTIES
    LINKER_LANGUAGE CXX
)

target_include_directories(
  ${SPORE_CODEGEN_TESTS}
  PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(
  ${SPORE_CODEGEN_TESTS}
  PRIVATE
    cppast
    fmt
    inja
    nlohmann_json
    spdlog
    Catch2::Catch2WithMain
)

list(
  APPEND CMAKE_MODULE_PATH
    ${CMAKE_BINARY_DIR}/content/catch2/extras
)

include(CTest)
include(Catch)

catch_discover_tests(${SPORE_CODEGEN_TESTS})