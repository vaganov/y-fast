cmake_minimum_required(VERSION 3.5)

project(yfast)

set(CMAKE_CXX_STANDARD 20)

find_package(absl CONFIG)

include_directories(include)

add_executable(yfast_example examples/yfast_example.cpp)

add_executable(benchmark test/benchmark.cpp)

target_link_libraries(benchmark PRIVATE absl::base absl::flat_hash_map)
