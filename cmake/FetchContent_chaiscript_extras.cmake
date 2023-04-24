include(FetchContent)

set(CONTENT_NAME "chaiscript_extras")
set(CONTENT_URL "https://github.com/chaiscript/chaiscript_extras.git")
set(CONTENT_TAG "v1.0.1")
set(CONTENT_DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}/content/${CONTENT_NAME}")

message(STATUS
  "Fetching ${CONTENT_NAME}:\n"
  "  URL: ${CONTENT_URL}\n"
  "  TAG: ${CONTENT_TAG}\n"
  "  DIRECTORY: ${CONTENT_DOWNLOAD_DIR}"
)

FetchContent_Declare(
  ${CONTENT_NAME}
  GIT_REPOSITORY  ${CONTENT_URL}
  GIT_TAG         ${CONTENT_TAG}
  SOURCE_DIR      ${CONTENT_DOWNLOAD_DIR}
)

FetchContent_MakeAvailable(${CONTENT_NAME})