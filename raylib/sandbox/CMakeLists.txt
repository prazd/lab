cmake_minimum_required(VERSION 3.15)
project(Sandbox)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(RAYLIB_DIR "/usr/local/Cellar/raylib/") 

find_package(raylib REQUIRED HINTS ${RAYLIB_DIR})

set(SOURCES
    main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
