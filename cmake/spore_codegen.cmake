function(spore_codegen SPORE_TARGET_NAME)
  cmake_parse_arguments(
    "SPORE_CODEGEN"
    "DEBUG;SEQUENTIAL;FORCE;REFORMAT"
    "CONFIG;CACHE;CXX_STANDARD;TARGET_NAME;BIN_NAME;DUMP_AST;OUTPUT;WORKING_DIRECTORY"
    "DEFINITIONS;INCLUDES;FEATURES;USER_DATA;TEMPLATE_PATHS"
    ${ARGN}
  )

  if (NOT SPORE_CODEGEN_BIN_NAME)
    set(SPORE_CODEGEN_BIN_NAME $<TARGET_FILE:${SPORE_CODEGEN_BINARY_TARGET}>)
  endif ()

  if (NOT SPORE_CODEGEN_TARGET_NAME)
    set(SPORE_CODEGEN_TARGET_NAME "${SPORE_TARGET_NAME}.codegen")
  endif ()

  if (NOT SPORE_CODEGEN_CXX_STANDARD)
    set(SPORE_CODEGEN_CXX_STANDARD $<TARGET_PROPERTY:${SPORE_TARGET_NAME},CXX_STANDARD>)
  endif ()

  if (NOT SPORE_CODEGEN_REFORMAT)
    find_package(ClangFormat)
    if (CLANG_FORMAT_FOUND)
      set(SPORE_CODEGEN_REFORMAT "${CLANG_FORMAT_EXECUTABLE} -i")
    else ()
      set(SPORE_CODEGEN_REFORMAT "false")
    endif ()
  endif ()

  if(NOT SPORE_CODEGEN_WORKING_DIRECTORY)
    set(SPORE_CODEGEN_WORKING_DIRECTORY $<TARGET_PROPERTY:${SPORE_TARGET_NAME},SOURCE_DIR>)
  endif()

  list(
    APPEND SPORE_CODEGEN_INCLUDES
      $<TARGET_PROPERTY:${SPORE_TARGET_NAME},INCLUDE_DIRECTORIES>
      $<TARGET_PROPERTY:${SPORE_TARGET_NAME},INTERFACE_INCLUDE_DIRECTORIES>
  )

  list(
    APPEND SPORE_CODEGEN_DEFINITIONS
      $<TARGET_PROPERTY:${SPORE_TARGET_NAME},COMPILE_DEFINITIONS>
      $<TARGET_PROPERTY:${SPORE_TARGET_NAME},INTERFACE_COMPILE_DEFINITIONS>
  )

  list(
    APPEND SPORE_CODEGEN_FEATURES
      $<TARGET_PROPERTY:${SPORE_TARGET_NAME},COMPILE_FEATURES>
      $<TARGET_PROPERTY:${SPORE_TARGET_NAME},INTERFACE_COMPILE_FEATURES>
  )

  add_custom_target(
    ${SPORE_CODEGEN_TARGET_NAME}
    COMMAND
      ${SPORE_CODEGEN_BIN_NAME}
        "$<$<BOOL:${SPORE_CODEGEN_CONFIG}>:--config;${SPORE_CODEGEN_CONFIG};>"
        "$<$<BOOL:${SPORE_CODEGEN_CACHE}>:--cache;${SPORE_CODEGEN_CACHE};>"
        "$<$<BOOL:${SPORE_CODEGEN_OUTPUT}>:--output;${SPORE_CODEGEN_OUTPUT};>"
        "$<$<BOOL:${SPORE_CODEGEN_CXX_STANDARD}>:--cpp-standard;${SPORE_CODEGEN_CXX_STANDARD};>"
        "$<$<BOOL:${SPORE_CODEGEN_REFORMAT}>:--reformat;${SPORE_CODEGEN_REFORMAT};>"
        "$<$<BOOL:${SPORE_CODEGEN_DUMP_AST}>:--dump-ast;${SPORE_CODEGEN_DUMP_AST};>"
        "$<$<BOOL:$<FILTER:${SPORE_CODEGEN_INCLUDES},EXCLUDE,^$>>:--includes;$<JOIN:${SPORE_CODEGEN_INCLUDES},;--includes;>>"
        "$<$<BOOL:$<FILTER:${SPORE_CODEGEN_DEFINITIONS},EXCLUDE,^$>>:--definitions;$<JOIN:${SPORE_CODEGEN_DEFINITIONS},;--definitions;>>"
        "$<$<BOOL:$<FILTER:${SPORE_CODEGEN_FEATURES},EXCLUDE,^$>>:--features;$<JOIN:${SPORE_CODEGEN_FEATURES},;--features;>>"
        "$<$<BOOL:$<FILTER:${SPORE_CODEGEN_TEMPLATE_PATHS},EXCLUDE,^$>>:--template-paths;$<JOIN:${SPORE_CODEGEN_TEMPLATE_PATHS},;--template-paths;>>"
        "$<$<BOOL:$<FILTER:${SPORE_CODEGEN_USER_DATA},EXCLUDE,^$>>:--user-data;$<JOIN:${SPORE_CODEGEN_USER_DATA},;--user-data;>>"
        "$<$<BOOL:${SPORE_CODEGEN_DEBUG}>:--debug>"
        "$<$<BOOL:${SPORE_CODEGEN_FORCE}>:--force>"
        "$<$<BOOL:${SPORE_CODEGEN_SEQUENTIAL}>:--sequential>"
    COMMENT "Running codegen for ${SPORE_TARGET_NAME} in ${SPORE_CODEGEN_OUTPUT}"
    WORKING_DIRECTORY "${SPORE_CODEGEN_WORKING_DIRECTORY}"
    COMMAND_EXPAND_LISTS
    VERBATIM
  )

  add_dependencies(
    ${SPORE_TARGET_NAME}
    ${SPORE_CODEGEN_TARGET_NAME}
  )
endfunction()
