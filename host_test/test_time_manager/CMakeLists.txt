cmake_minimum_required(VERSION 3.16)

include("../extra_dirs_common.cmake")

project(test_time_manager)

set(EXTRACT_PATTERNS_test_time_manager "*/time_manager/src/*.cpp" "*/time_manager/include/*.hpp")
set(REMOVE_PATTERNS_test_time_manager "*/time_manager/include/interfaces/*.hpp")

include("../coverage_common.cmake")
setup_gtest_coverage(test_time_manager test_time_manager.elf)
