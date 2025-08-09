set(TARGET_NAME ${PROJECT_NAME}-lib)

set(SPORE_CODEGEN_LIB ${TARGET_NAME} CACHE STRING "Target for the codegen library")

if (SPORE_WITH_CPP)
  list(APPEND TARGET_FILES ${CMAKE_CURRENT_SOURCE_DIR}/codegen_parser_cpp.cpp)
endif ()

if (SPORE_WITH_SPIRV)
  list(APPEND TARGET_FILES ${CMAKE_CURRENT_SOURCE_DIR}/codegen_parser_spirv.cpp)
endif ()

find_path(SPORE_PICOSHA2_INCLUDE_DIRS picosha2.h)

find_package(inja CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(tiny-process-library CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

add_library(${TARGET_NAME} ${TARGET_FILES})

target_link_libraries(
  ${TARGET_NAME} PUBLIC
  nlohmann_json::nlohmann_json
  pantor::inja
  spdlog::spdlog_header_only
  tiny-process-library::tiny-process-library
  yaml-cpp::yaml-cpp
)

target_include_directories(
  ${TARGET_NAME} PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
  ${SPORE_PICOSHA2_INCLUDE_DIRS}
)

set_target_properties(
  ${TARGET_NAME} PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}
)

if (SPORE_WITH_CPP)
  find_package(LLVM REQUIRED CONFIG)
  find_package(Clang REQUIRED CONFIG)

  target_include_directories(
    ${TARGET_NAME} PUBLIC
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
  )

  target_link_libraries(
    ${TARGET_NAME} PUBLIC
    clangTooling
    clangBasic
  )

  target_compile_definitions(
    ${TARGET_NAME} PUBLIC
    ${LLVM_DEFINITIONS}
    SPORE_WITH_CPP
  )
endif ()

if (SPORE_WITH_SPIRV)
  find_package(unofficial-spirv-reflect CONFIG REQUIRED)

  target_link_libraries(
    ${TARGET_NAME} PUBLIC
    unofficial::spirv-reflect
  )

  target_compile_definitions(
    ${TARGET_NAME} PUBLIC
    SPORE_WITH_SPIRV
  )
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  find_package(TBB REQUIRED)

  target_link_libraries(
    ${TARGET_NAME} PUBLIC
    TBB::tbb
  )
endif ()
