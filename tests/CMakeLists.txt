# Add GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)

# Function to add test executable and link libraries
function(add_gtest target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} ${GTEST_LIBRARIES} pthread)
    add_test(NAME ${target} COMMAND ${target})
endfunction()

# Get all .cpp files in the current directory
file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Loop through each .cpp file and create an executable target and test
foreach(TEST_FILE ${TEST_FILES})
    # Get the filename without the extension
    get_filename_component(TARGET_NAME ${TEST_FILE} NAME_WE)
    # Add the test executable and link libraries
    add_gtest(${TARGET_NAME} ${TEST_FILE})
endforeach()

# Enable testing
enable_testing()