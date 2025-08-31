cmake_minimum_required(VERSION 3.5)

project(yfast)

set(CMAKE_CXX_STANDARD 20)

find_package(absl CONFIG)
find_package(GTest CONFIG)

include_directories(include)

add_executable(yfast_example examples/yfast_example.cpp)

add_executable(benchmark test/benchmark.cpp)
target_link_libraries(benchmark PRIVATE absl::base absl::flat_hash_map)

add_executable(bst_unit_test test/unit/impl/bst.cpp)
target_compile_options(bst_unit_test PRIVATE --coverage)
target_link_options(bst_unit_test PRIVATE --coverage)
target_link_libraries(bst_unit_test PRIVATE gtest gtest_main)

add_executable(avl_unit_test test/unit/impl/avl.cpp)
target_compile_options(avl_unit_test PRIVATE --coverage)
target_link_options(avl_unit_test PRIVATE --coverage)
target_link_libraries(avl_unit_test PRIVATE gtest gtest_main)

add_executable(xfast_unit_test test/unit/impl/xfast.cpp)
target_compile_options(xfast_unit_test PRIVATE --coverage)
target_link_options(xfast_unit_test PRIVATE --coverage)
target_link_libraries(xfast_unit_test PRIVATE gtest gtest_main)

add_executable(yfast_unit_test test/unit/impl/yfast.cpp)
target_compile_options(yfast_unit_test PRIVATE --coverage)
target_link_options(yfast_unit_test PRIVATE --coverage)
target_link_libraries(yfast_unit_test PRIVATE gtest gtest_main)

add_executable(fastmap_unit_test test/unit/fastmap.cpp)
target_compile_options(fastmap_unit_test PRIVATE --coverage)
target_link_options(fastmap_unit_test PRIVATE --coverage)
target_link_libraries(fastmap_unit_test PRIVATE gtest gtest_main)

add_executable(uuid_fuzz_test test/fuzz/uuid.cpp)
target_compile_options(uuid_fuzz_test PRIVATE --coverage)
target_link_options(uuid_fuzz_test PRIVATE --coverage)
target_link_libraries(uuid_fuzz_test PRIVATE gtest gtest_main)

include(CTest OPTIONAL)
enable_testing()
add_test(NAME bst_unit_test COMMAND bst_unit_test)
add_test(NAME avl_unit_test COMMAND avl_unit_test)
add_test(NAME xfast_unit_test COMMAND xfast_unit_test)
add_test(NAME yfast_unit_test COMMAND yfast_unit_test)
add_test(NAME fastmap_unit_test COMMAND fastmap_unit_test)
add_test(NAME uuid_fuzz_test COMMAND uuid_fuzz_test)
