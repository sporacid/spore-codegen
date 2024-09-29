include(FindPackageMessage)

find_file(
  CLANG_LIBRARIES
  NAMES
    libclang.lib
    libclang.a
    libclang.so
  PATH_SUFFIXES bin lib
)

find_path(
  CLANG_INCLUDE_DIRS
  NAMES clang-c
  PATH_SUFFIXES include
)

if (CLANG_LIBRARIES AND CLANG_INCLUDE_DIRS)
  set(CLANG_FOUND TRUE)
  find_package_message(
    Clang
    "Clang was found, library=${CLANG_LIBRARIES} include=${CLANG_INCLUDE_DIRS}"
    "[${CLANG_LIBRARIES}][${CLANG_INCLUDE_DIRS}]"
  )
else ()
  set(CLANG_FOUND FALSE)
  if (Clang_FIND_REQUIRED)
    message(FATAL_ERROR "Clang was not found")
  else ()
    find_package_message(Clang "Clang was not found")
  endif ()
endif ()