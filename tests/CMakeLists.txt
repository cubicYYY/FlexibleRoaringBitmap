# Add GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)

# Testing files
add_executable(ArrayContainerTest test_array_container.cpp)
target_link_libraries(ArrayContainerTest ${GTEST_LIBRARIES} pthread)
add_executable(BitmapContainerTest test_bitmap_container.cpp)
target_link_libraries(BitmapContainerTest ${GTEST_LIBRARIES} pthread)
add_executable(RLEContainerTest test_rle_container.cpp)
target_link_libraries(RLEContainerTest ${GTEST_LIBRARIES} pthread)
add_executable(IntegrationTest integration_test.cpp)
target_link_libraries(IntegrationTest ${GTEST_LIBRARIES} pthread)

# Enable testing
enable_testing()

add_test(NAME ArrayContainerTest COMMAND ArrayContainerTest)
add_test(NAME BitmapContainerTest COMMAND BitmapContainerTest)
add_test(NAME RLEContainerTest COMMAND RLEContainerTest)
add_test(NAME IntegrationTest COMMAND IntegrationTest)