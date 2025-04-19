cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)


add_executable(VG_tests JSONToolsTest.cpp)

target_link_libraries(VG_tests gtest gtest_main VG JSON)

enable_testing()
add_test(NAME VG_tests COMMAND VG_tests)