cmake_minimum_required(VERSION 3.25)
project(GoL)

set(CMAKE_CXX_STANDARD 20)

# Установка флагов компилятора и компоновщика для санитайзеров и отладочной информации
set(SANITIZER_FLAGS "-fsanitize=address -fsanitize=undefined -g -O0")
#set(SANITIZER_FLAGS "-fsanitize=undefined -fsanitize=thread -g -O0")
# Применение флагов компилятора и компоновщика
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
find_package(SFML 2.6.1 COMPONENTS system window graphics network audio REQUIRED)

add_executable(GoL
        main.cpp)

target_include_directories(GoL PRIVATE /System/Library/Frameworks/OpenGL.framework/Headers)

target_link_libraries(GoL sfml-system sfml-window sfml-graphics sfml-network sfml-audio)
target_link_libraries(GoL "-framework OpenGL" "-framework GLUT")