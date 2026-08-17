cmake_minimum_required(VERSION 3.15)
project(Gravity)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SANITIZER_FLAGS "-O3")


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")

set(RAYLIB_DIR "/usr/local/Cellar/raylib/") 

find_package(raylib REQUIRED HINTS ${RAYLIB_DIR})

set(SOURCES
    main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
