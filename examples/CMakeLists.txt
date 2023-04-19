# Removes warning for undefined attributes
add_compile_options(
  "$<$<CXX_COMPILER_ID:MSVC>:/wd5030>"
  "$<$<CXX_COMPILER_ID:GNU>:-Wno-attributes>"
  "$<$<CXX_COMPILER_ID:Clang>:-Wno-attributes>"
)

add_subdirectory(enum)
add_subdirectory(json)