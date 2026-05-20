cmake_minimum_required(VERSION 3.15)
project(LindenmayerSystem)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(SANITIZER_FLAGS "-fsanitize=undefined")
set(OPTIMIZE_FLAGS "-O3 -march=native -fno-rtti")
#set(OPTIMIZE_FLAGS "-g -O0")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS} ${OPTIMIZE_FLAGS}")
set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} ${SANITIZER_FLAGS} ${OPTIMIZE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS} ${OPTIMIZE_FLAGS}")

set(RAYLIB_DIR "/usr/local/Cellar/raylib/") 

include_directories(include)

find_package(raylib REQUIRED HINTS ${RAYLIB_DIR})

set(SOURCES
    main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
