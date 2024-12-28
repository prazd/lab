cmake_minimum_required(VERSION 3.25)
project(Editor)

set(CMAKE_CXX_STANDARD 20)

set(SANITIZER_FLAGS "-fsanitize=address -fsanitize=undefined -g -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")

add_executable(Editor
        main.cpp)