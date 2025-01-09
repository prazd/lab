cmake_minimum_required(VERSION 3.25)
project(Editor)

set(CMAKE_CXX_STANDARD 20)

if (NOT DEFINED USE_VALGRIND)
    set(SANITIZER_FLAGS "-fsanitize=address -fsanitize=undefined -g -O1")
    set(CMAKE_CXX_FLAGS_DEBUG "${SANITIZER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${SANITIZER_FLAGS}")
endif()

add_executable(Editor
        main.cpp)