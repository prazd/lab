cmake_minimum_required(VERSION 3.15)
project(NES)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

 set(SANITIZER_FLAGS "-fsanitize=address -fsanitize=undefined")
# set(SANITIZER_FLAGS "-fsanitize=thread")

set(RAYLIB_DIR "/usr/local/Cellar/raylib/") 

include_directories(include)

find_package(raylib REQUIRED HINTS ${RAYLIB_DIR})

set(SOURCES
    src/Bus.cpp
    src/PPU.cpp
    src/CPU.cpp
    main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
