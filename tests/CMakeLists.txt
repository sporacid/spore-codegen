set(TARGET_NAME ${PROJECT_NAME}-tests)

if (SPORE_WITH_CPP)
  list(APPEND TARGET_FILES ${CMAKE_CURRENT_SOURCE_DIR}/t_codegen_parser_cpp.cpp)
endif ()

if (SPORE_WITH_SPIRV)
  list(APPEND TARGET_FILES ${CMAKE_CURRENT_SOURCE_DIR}/t_codegen_parser_spirv.cpp)
endif ()

add_executable(${TARGET_NAME} ${TARGET_FILES})

target_link_libraries(
  ${TARGET_NAME} PRIVATE
  ${SPORE_CODEGEN_LIB}
  Catch2::Catch2WithMain
)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR}/content/catch2/extras)

find_package(Catch2 CONFIG REQUIRED)

include(Catch)

catch_discover_tests(${TARGET_NAME})

if (BUILD_SHARED_LIBS)
  add_custom_command(
    TARGET ${TARGET_NAME} PRE_LINK
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:${SPORE_CODEGEN_LIB}>
    $<TARGET_FILE_DIR:${TARGET_NAME}>
  )
endif ()