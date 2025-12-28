cmake_minimum_required(VERSION 3.15)
project(Main)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(OPTIMIZE_FLAGS "-O2 -march=native -flto=thin -fno-rtti")
include_directories(include)

set(SOURCES
    src/Glsl.cpp
    main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
