#include <iostream>
#include <cassert>

extern char** environ;

// envp — это нестандартное расширение
// Стандарт C++ (и даже стандарт C) не описывает третий аргумент main.
// Он определён только в некоторых системах (POSIX, GNU libc, BSD и т.п.) как

// environ — тоже не из стандарта C++
// Это глобальная переменная (тип char**), определяемая в POSIX и некоторых других средах.
// Для её использования обычно нужно: - #include <unistd.h>

// envp == environ — не гарантируется
// В большинстве реализаций на Unix/Linux это одно и то же (оба указывают на начало массива указателей на строки окружения).
// Но стандарт POSIX не требует, чтобы это были одни и те же указатели. Они могут быть разными, даже если содержимое идентично.
int main(int argc, char*  argv[], char* envp[]) { 
    std::cout << "getenv:" << std::getenv("DATA") << '\n';
    std::cout << "envp address:" << (void*)envp << '\n';
    std::cout << "environ address:" << (void*)environ << '\n';

    std::cout << "Full envp:\n";
    for (auto it = envp; *it != nullptr; ++it)
        std::cout << *it << '\n';
}