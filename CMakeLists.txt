cmake_minimum_required(VERSION 3.6)

project(nan-linux)

if (CMAKE_C_COMPILER_ID MATCHES "GNU" OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_compile_options("-Wall" "-Wextra")
endif ()
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

add_library(radiotap radiotap/radiotap.c)
target_compile_definitions(radiotap PRIVATE -D_DEFAULT_SOURCE -DRADIOTAP_SUPPORT_OVERRIDES)

add_subdirectory(src)
add_subdirectory(daemon)

#add_subdirectory(googletest)
#add_subdirectory(tests)

install(TARGETS radiotap DESTINATION lib)
