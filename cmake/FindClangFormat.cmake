include(FindPackageMessage)

find_program(
  CLANG_FORMAT_EXECUTABLE
  NAMES clang-format
  PATH_SUFFIXES bin
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
  if (ClangFormat_FIND_REQUIRED)
    message(FATAL_ERROR "Clang format was not found")
  else ()
    find_package_message(Clang "Clang format was not found")
  endif ()
endif ()
