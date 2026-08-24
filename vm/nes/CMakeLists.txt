cmake_minimum_required(VERSION 3.15)
project(NES)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(SANITIZER_FLAGS "-fsanitize=undefined")
#set(OPTIMIZE_FLAGS "-O3 -march=native -fno-rtti")
set(OPTIMIZE_FLAGS "-g -O0")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS} ${OPTIMIZE_FLAGS}")
set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} ${SANITIZER_FLAGS} ${OPTIMIZE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS} ${OPTIMIZE_FLAGS}")

set(RAYLIB_DIR "/usr/local/Cellar/raylib/") 

include_directories(include)

find_package(raylib REQUIRED HINTS ${RAYLIB_DIR})
find_package(SDL2 REQUIRED COMPONENTS SDL2)

set(SOURCES
    src/Bus.cpp
    src/PPU.cpp
    src/APU.cpp
    src/CPU.cpp
    main.cpp
)

#set_source_files_properties(
#        src/PPU.cpp
#        PROPERTIES
#        COMPILE_OPTIONS "-g;-O0"
#)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE
        raylib
        SDL2::SDL2
        "$<LINK_LIBRARY:FRAMEWORK,IOKit>"
        "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation>"
        "$<LINK_LIBRARY:FRAMEWORK,Foundation>"
)
