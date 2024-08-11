include(FindPackageMessage)

if (DEFINED LLVM_DIR)
  set(_LLVM_DIR ${LLVM_DIR})
elseif (DEFINED ENV{LLVM_DIR})
  set(_LLVM_DIR $ENV{LLVM_DIR})
endif ()

if (DEFINED _LLVM_DIR)
  set(
    CLANG_LIBRARY_PATHS
      ${_LLVM_DIR}/bin
      ${_LLVM_DIR}/bin/clang
      ${_LLVM_DIR}/lib
      ${_LLVM_DIR}/lib/clang
  )

  set(
    CLANG_INCLUDE_PATHS
      ${_LLVM_DIR}/include
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
  find_package_message(
    Clang
    "Clang was found, library=${CLANG_LIBRARIES} include=${CLANG_INCLUDE_DIRS}"
    "[${CLANG_LIBRARIES}][${CLANG_INCLUDE_DIRS}]"
  )
else ()
  set(CLANG_FOUND FALSE)
  if (CLANG_REQUIRED)
    message(FATAL_ERROR "Clang was not found")
  else ()
    find_package_message(
      Clang
      "Clang was not found, directory=${_LLVM_DIR}"
      "[${_LLVM_DIR}]"
    )
  endif ()
endif ()

unset(_LLVM_DIR)
unset(CLANG_LIBRARY_PATHS)
unset(CLANG_INCLUDE_PATHS)