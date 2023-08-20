include(FetchContent)

set(CONTENT_NAME "glob")
set(CONTENT_URL "https://github.com/p-ranav/glob.git")
set(CONTENT_TAG "master")

message(
  STATUS "Fetching ${CONTENT_NAME}: ${CONTENT_URL} TAG: ${CONTENT_TAG}"
)

FetchContent_Declare(
  ${CONTENT_NAME}
  GIT_REPOSITORY ${CONTENT_URL}
  GIT_TAG ${CONTENT_TAG}
)

FetchContent_MakeAvailable(${CONTENT_NAME})