cmake_minimum_required(VERSION 3.5)

set(SRC_DIR src)
set(TEST_DIR test)
set(UNIT_TEST_DIR ${TEST_DIR}/unit)

set(CMAKE_CXX_FLAGS "-std=c++11 -g")

include_directories(${SRC_DIR})

add_executable(xfast.t ${UNIT_TEST_DIR}/xfast.t.cpp)

target_link_libraries(xfast.t gtest gtest_main pthread)
