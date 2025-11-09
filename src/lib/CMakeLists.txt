set(TARGET_NAME ${PROJECT_NAME}-lib)

set(SPORE_CODEGEN_LIB ${TARGET_NAME} CACHE STRING "Target for the codegen library")

if (SPORE_WITH_CPP)
  list(APPEND TARGET_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_parser_cpp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_formatter_cpp.cpp)
endif ()

if (SPORE_WITH_SPIRV)
  list(APPEND TARGET_FILES ${CMAKE_CURRENT_SOURCE_DIR}/codegen_parser_spirv.cpp)
endif ()

if (SPORE_WITH_SLANG)
  list(APPEND TARGET_FILES ${CMAKE_CURRENT_SOURCE_DIR}/codegen_parser_slang.cpp)
endif ()

find_path(SPORE_PICOSHA2_INCLUDE_DIRS picosha2.h)

find_package(inja CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

add_library(${TARGET_NAME} STATIC ${TARGET_FILES})

target_link_libraries(
  ${TARGET_NAME} PUBLIC
  pantor::inja
  nlohmann_json::nlohmann_json
  spdlog::spdlog_header_only
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

if (BUILD_SHARED_LIBS)
  set_target_properties(
    ${TARGET_NAME} PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
  )
endif ()

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
  target_link_libraries(${TARGET_NAME} PUBLIC unofficial::spirv-reflect)
  target_compile_definitions(${TARGET_NAME} PUBLIC SPORE_WITH_SPIRV)
endif ()

if (SPORE_WITH_SLANG)
  find_path(SPORE_SLANG_INCLUDE_DIRS slang.h)
  find_library(SPORE_SLANG_LIB_DIRS slang)
  target_include_directories(${TARGET_NAME} PUBLIC ${SPORE_SLANG_INCLUDE_DIRS})
  target_link_libraries(${TARGET_NAME} PUBLIC ${SPORE_SLANG_LIB_DIRS})
  target_compile_definitions(${TARGET_NAME} PUBLIC SPORE_WITH_SLANG)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
  find_package(TBB REQUIRED)
  target_link_libraries(${TARGET_NAME} PUBLIC TBB::tbb)
endif ()
