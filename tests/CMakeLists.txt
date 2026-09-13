cmake_minimum_required(VERSION 3.16)
project(cyberputer_host_tests LANGUAGES CXX)
enable_testing()
add_executable(visualization_test visualization_test.cpp
    ../firmware/src/core/visualization/observations.cpp
    ../firmware/src/core/visualization/soundtrack.cpp
    ../firmware/src/core/visualization/renderer.cpp)
target_include_directories(visualization_test PRIVATE ../firmware/src)
target_compile_features(visualization_test PRIVATE cxx_std_17)
target_compile_options(visualization_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(visualization_test PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(visualization_test PRIVATE -fsanitize=address,undefined)
endif()
add_test(NAME visualization COMMAND visualization_test)
