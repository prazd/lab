#include <iostream>

// Когда использовать constinit?
// - Если переменная должна быть обязательно инициализирована на этапе компиляции.
// - Для глобальных/статических переменных, чтобы избежать data race при ленивой инициализации.
// - Если нужно явно указать намерение разработчика использовать compile-time инициализацию.

constexpr int getnum() {return 42;}
constinit int a = getnum();

int main() {
    static constinit int b = getnum();
    // constinit int c = getnum(); error
    std::cout << a << "\n";
    std::cout << b << "\n";
}