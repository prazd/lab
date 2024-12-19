cmake_minimum_required(VERSION 3.25)
project(main)

set(CMAKE_CXX_STANDARD 20)

# Установка флагов компилятора и компоновщика для санитайзеров и отладочной информации
set(SANITIZER_FLAGS "-fsanitize=address -fsanitize=undefined -g -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")

# Указываем путь к GMP
set(GMP_INCLUDE_DIR "/usr/local/Cellar/gmp/6.3.0/include")
set(GMP_LIBRARIES "/usr/local/Cellar/gmp/6.3.0/lib/libgmp.dylib")

# Установка основного исполняемого файла
add_executable(main main.cpp)

# Подключение заголовочных файлов и библиотек GMP
target_include_directories(main PRIVATE ${GMP_INCLUDE_DIR})
target_link_libraries(main PRIVATE ${GMP_LIBRARIES})
