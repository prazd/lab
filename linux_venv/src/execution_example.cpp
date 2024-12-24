#include <iostream>
#include <vector>
#include <algorithm>
#include <execution>

int main() {
    std::vector<int> data(1'000'000, 1);

    // Последовательное выполнение
    std::for_each(std::execution::seq, data.begin(), data.end(), [](int& x) {
        x += 1;
    });

    // Параллельное выполнение
    std::for_each(std::execution::par, data.begin(), data.end(), [](int& x) {
        x *= 2;
    });

    // Параллельное выполнение с векторизацией
    std::for_each(std::execution::par_unseq, data.begin(), data.end(), [](int& x) {
        x -= 1;
    });

    std::cout << "First element: " << data[0] << std::endl;

    return 0;
}
