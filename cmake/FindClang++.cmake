#
# .rst: FindClangFormat
# ---------------
#
# The module defines the following variables
#
# ``CLANG_PP_EXECUTABLE`` Path to clang++ executable
# ``CLANG_PP_FOUND`` True if the clang++ executable was found.
# ``CLANG_PP_VERSION`` The version of clang++ found
# ``CLANG_PP_VERSION_MAJOR`` The clang++ major version if specified, 0
# otherwise ``CLANG_PP_VERSION_MINOR`` The clang++ minor version if
# specified, 0 otherwise ``CLANG_PP_VERSION_PATCH`` The clang++ patch
# version if specified, 0 otherwise ``CLANG_PP_VERSION_COUNT`` Number of
# version components reported by clang++
#
# Example usage:
#
# .. code-block:: cmake
#
# find_package(ClangFormat) if(CLANG_PP_FOUND) message("clang++
# executable found: ${CLANG_PP_EXECUTABLE}\n" "version:
# ${CLANG_PP_VERSION}") endif()

find_program(CLANG_PP_EXECUTABLE
  NAMES
    clangpp
    clang++
    clang++-3
    clang++-4
    clang++-5
    clang++-6
    clang++-7
    clang++-8
    clang++-9
    clang++-10
    clang++-11
    clang++-12
    clang++-13
    clang++-14
    clang++-15
    clang++-16
    clang++-17
    clang++-18
  DOC "clang++ executable")
mark_as_advanced(CLANG_PP_EXECUTABLE)

# Extract version from command "clang++ -version"
if(CLANG_PP_EXECUTABLE)
  execute_process(COMMAND ${CLANG_PP_EXECUTABLE} -version
    OUTPUT_VARIABLE clang_pp_version
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(clang_pp_version MATCHES "^clang version .*")
    # clang_pp_version sample: "clang++ version 3.9.1-4ubuntu3~16.04.1
    # (tags/RELEASE_391/rc2)"
    string(REGEX
      REPLACE "clang version ([.0-9]+).*"
      "\\1"
      CLANG_PP_VERSION
      "${clang_pp_version}")
    # CLANG_PP_VERSION sample: "3.9.1"

    # Extract version components
    string(REPLACE "."
      ";"
      clang_pp_version
      "${CLANG_PP_VERSION}")
    list(LENGTH clang_pp_version CLANG_PP_VERSION_COUNT)
    if(CLANG_PP_VERSION_COUNT GREATER 0)
      list(GET clang_pp_version 0 CLANG_PP_VERSION_MAJOR)
    else()
      set(CLANG_PP_VERSION_MAJOR 0)
    endif()
    if(CLANG_PP_VERSION_COUNT GREATER 1)
      list(GET clang_pp_version 1 CLANG_PP_VERSION_MINOR)
    else()
      set(CLANG_PP_VERSION_MINOR 0)
    endif()
    if(CLANG_PP_VERSION_COUNT GREATER 2)
      list(GET clang_pp_version 2 CLANG_PP_VERSION_PATCH)
    else()
      set(CLANG_PP_VERSION_PATCH 0)
    endif()
  endif()
  unset(clang_pp_version)
endif()

if(CLANG_PP_EXECUTABLE)
  set(CLANG_PP_FOUND TRUE)
else()
  set(CLANG_PP_FOUND FALSE)
endif()
