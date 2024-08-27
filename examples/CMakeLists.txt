# Add executable target for example programs
add_executable(example example.cpp)

# Include directory for the example program
target_include_directories(example PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ../lib)
