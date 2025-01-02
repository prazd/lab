#include <iostream>
#include <vector>

template <typename T>
void print(const T& ctr) {
    for (const auto& el : ctr)
        std::cout << el << " ";
    std::cout << "\n";
}


void print_sierpinski_trianle(const auto& triangle) {
    std::string spaces(triangle.back().size(), ' ');
    for(auto& row: triangle) {
        std::cout << spaces;
        for(auto& el: row){
            if (el % 2 == 0) {
                std::cout << "\x1B[31m"; // red
                std::cout << '@' << " ";
                std::cout << "\x1B[37m";
            } else {
                std::cout << '@' << " ";
            }
        }
        std::cout << "\n";
        spaces.pop_back();
    }
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
    print_sierpinski_trianle(create_pascal_triangle(30));
}
