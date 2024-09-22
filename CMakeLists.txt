cmake_minimum_required(VERSION 3.10)
project(FlexibleRoaring)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# Add subdirectories
add_subdirectory(examples)
add_subdirectory(tests)
