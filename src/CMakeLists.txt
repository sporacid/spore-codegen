set(
  SPORE_CODEGEN_BINARY_TARGET "${PROJECT_NAME}-bin"
  CACHE STRING "Target for the main codegen application"
)

set(
  SPORE_CODEGEN_LIBRARY_TARGET "${PROJECT_NAME}-lib"
  CACHE STRING "Target for the main codegen library"
)

set(
  SPORE_CODEGEN_BINARY_NAME "spore-codegen"
  CACHE STRING "Output name for the main codegen application"
)

add_library(
  ${SPORE_CODEGEN_LIBRARY_TARGET}
  INTERFACE
)

find_path(SPORE_PICOSHA2_INCLUDE_DIRS "picosha2.h")

find_package(argparse CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(inja CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(p-ranav-glob CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(tiny-process-library CONFIG REQUIRED)
find_package(Clang CONFIG REQUIRED)

target_link_libraries(
  ${SPORE_CODEGEN_LIBRARY_TARGET}
  INTERFACE
    fmt::fmt
    nlohmann_json::nlohmann_json
    pantor::inja
    p-ranav::glob
    spdlog::spdlog
    tiny-process-library::tiny-process-library
    libclang
)

target_include_directories(
  ${SPORE_CODEGEN_LIBRARY_TARGET}
  INTERFACE
    ${PROJECT_SOURCE_DIR}/include
    ${SPORE_PICOSHA2_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  find_package(TBB REQUIRED)
  target_link_libraries(
    ${SPORE_CODEGEN_LIBRARY_TARGET}
    INTERFACE
      TBB::tbb
  )
endif ()

file(
  GLOB_RECURSE SPORE_CODEGEN_HEADER_FILES
  ${PROJECT_SOURCE_DIR}/include/**.hpp
)

file(
  GLOB_RECURSE SPORE_CODEGEN_SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/src/**.cpp
)

add_executable(
  ${SPORE_CODEGEN_BINARY_TARGET}
  ${SPORE_CODEGEN_HEADER_FILES}
  ${SPORE_CODEGEN_SOURCE_FILES}
)

set_target_properties(
  ${SPORE_CODEGEN_BINARY_TARGET}
  PROPERTIES
    LINKER_LANGUAGE CXX
    OUTPUT_NAME ${SPORE_CODEGEN_BINARY_NAME}
)

target_link_libraries(
  ${SPORE_CODEGEN_BINARY_TARGET}
  PRIVATE
    ${SPORE_CODEGEN_LIBRARY_TARGET}
    argparse::argparse
)

include(GNUInstallDirs)

install(
  TARGETS ${SPORE_CODEGEN_BINARY_TARGET}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
  DIRECTORY ${PROJECT_SOURCE_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "**.hpp"
)

install(
  FILES
  ${PROJECT_SOURCE_DIR}/cmake/SporeCodegen.cmake
  ${PROJECT_SOURCE_DIR}/cmake/FindClangFormat.cmake
  DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake
)