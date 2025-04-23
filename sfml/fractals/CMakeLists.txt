cmake_minimum_required(VERSION 3.25)
project(SierpinskiTriangle)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_PREFIX_PATH "$ENV{HOME}/Desktop/lib/SFML/build-static") # 2.6.1
set(SFML_STATIC_LIBRARIES TRUE)
include_directories(${SFML_INCLUDE_DIR})


# set(SANITIZER_FLAGS "-fsanitize=address -fsanitize=undefined -g -O0")
set(SANITIZER_FLAGS "-fsanitize=undefined -fsanitize=thread -g -O0")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
find_package(SFML 2.6.1 COMPONENTS system window graphics network audio REQUIRED)

add_executable(FractalsGenerator
        main.cpp)

target_include_directories(FractalsGenerator PRIVATE /System/Library/Frameworks/OpenGL.framework/Headers)

target_link_libraries(FractalsGenerator sfml-graphics sfml-network sfml-audio)
target_link_libraries(FractalsGenerator "-framework OpenGL" "-framework GLUT")