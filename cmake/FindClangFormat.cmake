include(FindPackageMessage)

if (DEFINED LLVM_DIR)
  set(_LLVM_DIR ${LLVM_DIR})
elseif (DEFINED ENV{LLVM_DIR})
  set(_LLVM_DIR $ENV{LLVM_DIR})
endif ()

if (DEFINED _LLVM_DIR)
  set(
    CLANG_EXECUTABLE_PATHS
      ${_LLVM_DIR}/bin
  )
endif ()

find_program(
  CLANG_FORMAT_EXECUTABLE
  NAMES clang-format
  PATHS ${CLANG_EXECUTABLE_PATHS}
)

if (CLANG_FORMAT_EXECUTABLE)
  set(CLANG_FORMAT_FOUND TRUE)
  find_package_message(
    ClangFormat
    "Clang format was found, executable=${CLANG_FORMAT_EXECUTABLE}"
    "[${CLANG_FORMAT_EXECUTABLE}]"
  )
else ()
  set(CLANG_FORMAT_FOUND FALSE)
  if (CLANG_FORMAT_REQUIRED)
    message(FATAL_ERROR "Clang format was not found")
  else ()
    find_package_message(
      Clang
      "Clang format was not found, directory=${_LLVM_DIR}"
      "[${_LLVM_DIR}]"
    )
  endif ()
endif ()

unset(_LLVM_DIR)
unset(CLANG_EXECUTABLE_PATHS)