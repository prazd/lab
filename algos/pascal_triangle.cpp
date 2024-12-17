#include <iostream>
#include <vector>

template <typename T>
void print(const T& ctr) {
    for (const auto& el : ctr)
        std::cout << el << " ";
    std::cout << "\n";
}

auto create_pascal_triangle(int num) {
    std::vector<std::vector<int>> result(num);
    for (size_t i = 0; i < num; ++i) {
        result[i].resize(i + 1, 1);
        for (size_t j = 1; j < i; ++j) {
            result[i][j] = result[i - 1][j - 1] + result[i - 1][j];
        }
    }
    return result;
}

int main() {
    for (const auto& row : create_pascal_triangle(30)) {
        print(row);
    }
}
