# tests/CMakeLists.txt

# 添加 Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/lib)

# 添加测试文件
add_executable(ArrayContainerTest test_array_container.cpp)
target_link_libraries(ArrayContainerTest ${GTEST_LIBRARIES} pthread)
add_executable(BitmapContainerTest test_bitmap_container.cpp)
target_link_libraries(BitmapContainerTest ${GTEST_LIBRARIES} pthread)
add_executable(RLEContainerTest test_rle_container.cpp)
target_link_libraries(RLEContainerTest ${GTEST_LIBRARIES} pthread)

# 启用测试
enable_testing()

# 添加测试
add_test(NAME ArrayContainerTest COMMAND ArrayContainerTest)
add_test(NAME BitmapContainerTest COMMAND BitmapContainerTest)
add_test(NAME RLEContainerTest COMMAND RLEContainerTest)