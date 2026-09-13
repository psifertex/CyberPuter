cmake_minimum_required(VERSION 3.16)
project(cyberputer_host_tests LANGUAGES CXX)
enable_testing()
add_executable(visualization_test visualization_test.cpp
    ../firmware/src/core/detection/device_classifier.cpp
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

add_executable(device_classifier_test device_classifier_test.cpp
    ../firmware/src/core/detection/device_classifier.cpp)
add_executable(soundtrack_decode_test soundtrack_decode_test.cpp
    ../firmware/src/core/visualization/wav_stream.cpp)
foreach(test_target device_classifier_test soundtrack_decode_test)
    target_include_directories(${test_target} PRIVATE ../firmware/src)
    target_compile_features(${test_target} PRIVATE cxx_std_17)
    target_compile_options(${test_target} PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${test_target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${test_target} PRIVATE -fsanitize=address,undefined)
    endif()
endforeach()
add_test(NAME device_classifier COMMAND device_classifier_test)
add_test(NAME soundtrack COMMAND soundtrack_decode_test
    ${CMAKE_CURRENT_SOURCE_DIR}/../sdcard/cyberputer/music/01-Cybershaman.wav
    ${CMAKE_CURRENT_SOURCE_DIR}/../sdcard/cyberputer/music/02-Cyberpunk-Moonlight-Sonata.wav)
