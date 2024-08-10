if (DEFINED ENV{LLVM_DIR})
  set(
    CLANG_EXECUTABLE_PATHS
    $ENV{LLVM_DIR}/bin
  )
endif ()

find_program(
  CLANG_FORMAT_EXECUTABLE
  NAMES clang-format
  PATHS ${CLANG_EXECUTABLE_PATHS}
)

if (CLANG_FORMAT_EXECUTABLE)
  set(CLANG_FORMAT_FOUND TRUE)
  message(STATUS "Clang format was found, executable=${CLANG_FORMAT_EXECUTABLE}")
else ()
  set(CLANG_FORMAT_FOUND FALSE)
  if (CLANG_FORMAT_REQUIRED)
    message(FATAL_ERROR "Clang format was not found")
  else()
    message(STATUS "Clang format was not found")
  endif ()
endif ()