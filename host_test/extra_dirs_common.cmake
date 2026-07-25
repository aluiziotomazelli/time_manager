# host_test/extra_dirs_common.cmake
# Common setup for host-based test projects.

# The project root is one level up from this file (host_test/)
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# Append extra component directories so IDF can find our local components.
list(APPEND EXTRA_COMPONENT_DIRS
    "${PROJECT_ROOT}"
    "${PROJECT_ROOT}/host_test/gtest"
    "${PROJECT_ROOT}/../idf_hals"
    "$ENV{IDF_PATH}/tools/mocks/driver"
    "$ENV{IDF_PATH}/tools/mocks/esp_netif"
    "$ENV{IDF_PATH}/tools/mocks/esp_timer"
    "$ENV{IDF_PATH}/tools/mocks/lwip"
)

# Explicitly list the components to be included in the build.
set(COMPONENTS 
    "main"
    "time_manager"
    "idf_hals"
    "gtest"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
