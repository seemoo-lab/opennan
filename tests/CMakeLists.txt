include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(tests "" test_state.cpp)

target_sources(tests PRIVATE
        test_crc32.cpp
        test_sync.cpp
        )

target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(tests gtest gtest_main)
target_link_libraries(tests nanlib)