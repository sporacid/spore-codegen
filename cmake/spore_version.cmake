function(spore_get_version SPORE_VERSION_OUT)
  set(SPORE_VERSION_MAJOR 0)
  set(SPORE_VERSION_MINOR 0)
  set(SPORE_VERSION_PATCH 0)

  find_package(Git)

  if (GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} --no-pager log --format=%H
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE SPORE_GIT_COMMITS
    )

    string(
      REGEX REPLACE
      "[\r\n\t ]+"
      ";"
      SPORE_GIT_COMMITS
      ${SPORE_GIT_COMMITS}
    )

    foreach (SPORE_GIT_COMMIT ${SPORE_GIT_COMMITS})
      execute_process(
        COMMAND ${GIT_EXECUTABLE} --no-pager log -n1 --format=%B ${SPORE_GIT_COMMIT}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE SPORE_GIT_COMMIT_BODY
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )

      if (${SPORE_GIT_COMMIT_BODY} MATCHES ".*BREAKING CHANGES:.*$" OR ${SPORE_GIT_COMMIT_BODY} MATCHES "^[a-zA-Z0-9-_]+(\([a-zA-Z0-9-_]+\))?!:")
        math(EXPR SPORE_VERSION_MAJOR "${SPORE_VERSION_MAJOR}+1")
      elseif (${SPORE_GIT_COMMIT_BODY} MATCHES "^feat(\([a-zA-Z0-9-_]+\))?:")
        math(EXPR SPORE_VERSION_MINOR "${SPORE_VERSION_MINOR}+1")
      elseif (${SPORE_GIT_COMMIT_BODY} MATCHES "^fix(\([a-zA-Z0-9-_]+\))?:")
        math(EXPR SPORE_VERSION_PATCH "${SPORE_VERSION_PATCH}+1")
      endif ()
    endforeach ()
  endif ()

  set(${SPORE_VERSION_OUT} "${SPORE_VERSION_MAJOR}.${SPORE_VERSION_MINOR}.${SPORE_VERSION_PATCH}" PARENT_SCOPE)
endfunction()
