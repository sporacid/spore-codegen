if (DEFINED ENV{LLVM_DIR})
  set(
    CLANG_LIBRARY_PATHS
      $ENV{LLVM_DIR}/bin
      $ENV{LLVM_DIR}/bin/clang
      $ENV{LLVM_DIR}/lib
      $ENV{LLVM_DIR}/lib/clang
  )

  set(
    CLANG_INCLUDE_PATHS
      $ENV{LLVM_DIR}/include
  )
endif ()

find_file(
  CLANG_LIBRARIES
  NAMES
    libclang.lib
    libclang.a
    libclang.so
  PATHS ${CLANG_LIBRARY_PATHS}
)

find_path(
  CLANG_INCLUDE_DIRS
  NAMES clang-c
  PATHS ${CLANG_INCLUDE_PATHS}
)

if (CLANG_LIBRARIES AND CLANG_INCLUDE_DIRS)
  set(CLANG_FOUND TRUE)
  message(STATUS "Clang was found, libraries=${CLANG_LIBRARIES} includes=${CLANG_INCLUDE_DIRS}")
else ()
  set(CLANG_FOUND FALSE)
  if (CLANG_REQUIRED)
    message(FATAL_ERROR "Clang was not found")
  else()
    message(STATUS "Clang was not found")
  endif ()
endif ()