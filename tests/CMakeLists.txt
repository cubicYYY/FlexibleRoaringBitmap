# # Enable testing
# enable_testing()

# # Add test executables
# add_executable(test_containers test_containers.cpp)
# add_executable(test_rlecontainer test_rlecontainer.cpp)
# add_executable(test_arraycontainer test_arraycontainer.cpp)
# add_executable(test_bitmapcontainer test_bitmapcontainer.cpp)
# add_executable(test_flexibleroaringbitmap test_flexibleroaringbitmap.cpp)

# # Link the test executables with the library
# target_link_libraries(test_containers FlexibleRoaringBitmap)
# target_link_libraries(test_rlecontainer FlexibleRoaringBitmap)
# target_link_libraries(test_arraycontainer FlexibleRoaringBitmap)
# target_link_libraries(test_bitmapcontainer FlexibleRoaringBitmap)
# target_link_libraries(test_flexibleroaringbitmap FlexibleRoaringBitmap)

# # Include directories for the tests
# target_include_directories(test_containers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# target_include_directories(test_rlecontainer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# target_include_directories(test_arraycontainer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# target_include_directories(test_bitmapcontainer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# target_include_directories(test_flexibleroaringbitmap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# # Add tests to CTest
# add_test(NAME test_containers COMMAND test_containers)
# add_test(NAME test_rlecontainer COMMAND test_rlecontainer)
# add_test(NAME test_arraycontainer COMMAND test_arraycontainer)
# add_test(NAME test_bitmapcontainer COMMAND test_bitmapcontainer)
# add_test(NAME test_flexibleroaringbitmap COMMAND test_flexibleroaringbitmap)
