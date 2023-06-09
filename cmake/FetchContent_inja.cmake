include(FetchContent)

set(CONTENT_NAME "inja")
set(CONTENT_URL "https://github.com/pantor/inja.git")
set(CONTENT_TAG "v3.4.0")
set(CONTENT_DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}/content/${CONTENT_NAME}")

set(INJA_USE_EMBEDDED_JSON OFF CACHE INTERNAL INJA_USE_EMBEDDED_JSON)
set(BUILD_BENCHMARK OFF CACHE INTERNAL INJA_BUILD_BENCHMARK)

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