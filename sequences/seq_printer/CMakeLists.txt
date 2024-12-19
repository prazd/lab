cmake_minimum_required(VERSION 3.25)
project(SeqPrint)

set(CMAKE_CXX_STANDARD 20)

# Установка флагов компилятора и компоновщика для санитайзеров и отладочной информации
set(SANITIZER_FLAGS "-fsanitize=address -fsanitize=undefined -g -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")

# Указываем путь к GMP
set(GMP_INCLUDE_DIR "/usr/local/Cellar/gmp/6.3.0/include")
set(GMP_LIBRARY_DIR "/usr/local/Cellar/gmp/6.3.0/lib/")
# find_package(GMP REQUIRED)
# find_package(GMPXX REQUIRED)

# Установка основного исполняемого файла
add_executable(SeqPrint main.cpp)

# Подключение заголовочных файлов и библиотек GMP
target_include_directories(SeqPrint PRIVATE ${GMP_INCLUDE_DIR})
target_link_directories(SeqPrint PRIVATE ${GMP_LIBRARY_DIR})
target_link_libraries(SeqPrint PRIVATE gmp gmpxx)
