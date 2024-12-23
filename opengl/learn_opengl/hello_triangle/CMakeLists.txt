cmake_minimum_required(VERSION 3.15)
project(Test)

set(CMAKE_CXX_STANDARD 20)

# Указание пути к glfw3
set(CMAKE_PREFIX_PATH "/usr/local/Cellar/glfw/3.4/lib/cmake/glfw3/")
find_package(glfw3 REQUIRED)

# Указание пути к glm
set(GLM_INCLUDE_DIR "/usr/local/Cellar/glm/1.0.1/include")
include_directories(${GLM_INCLUDE_DIR})

# Указание пути к glad
include_directories("/Users/prazd/lib/glad/include")

add_executable(Test main.cpp)

# Линковка GLFW, OpenGL и glad
target_link_libraries(Test PRIVATE glfw "-framework OpenGL")

# Отключение предупреждений об устаревшем OpenGL на macOS
target_compile_definitions(Test PRIVATE GL_SILENCE_DEPRECATION)

# Линковка с glad
target_sources(Test PRIVATE "/Users/prazd/lib/glad/src/glad.c")
