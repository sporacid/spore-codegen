include(FetchContent)

set(CONTENT_NAME "cppast")
set(CONTENT_URL "https://github.com/foonathan/cppast.git")
set(CONTENT_TAG "main")

message(
  STATUS "Fetching ${CONTENT_NAME}: ${CONTENT_URL} TAG: ${CONTENT_TAG}"
)

FetchContent_Declare(
  ${CONTENT_NAME}
  GIT_REPOSITORY ${CONTENT_URL}
  GIT_TAG ${CONTENT_TAG}
)

FetchContent_MakeAvailable(${CONTENT_NAME})