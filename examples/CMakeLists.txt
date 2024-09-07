# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name
project(ExampleProject)

# Function to add executable and include directories
function(add_example target source)
    add_executable(${target} ${source})
    target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ../include)
endfunction()

# Get all .cpp files in the current directory
file(GLOB CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Loop through each .cpp file and create an executable target
foreach(CPP_FILE ${CPP_FILES})
    # Get the filename without the extension
    get_filename_component(TARGET_NAME ${CPP_FILE} NAME_WE)
    # Add the executable target
    add_example(${TARGET_NAME} ${CPP_FILE})
endforeach()