#include <iostream>
#include <array>

constexpr size_t HEIGHT = 30;
constexpr size_t WIDTH = 50;

template <size_t height, size_t width>
void print(const auto& field) {
    for (int i = 0; i < height; ++i){
        for (int j = 0; j < width; ++j)
            std::cout << (field[(width * i) + j] ? '0' : '.');
        std::cout << '\n';
    }
}

template <size_t height, size_t width>
consteval auto draw_line(std::pair<int, int> p0, std::pair<int, int> p1) {
    std::array<bool, height * width> arr{};
    auto dx = p1.first - p0.first;
    auto dy = p1.second - p0.second;
    auto dx2 = dx * 2;
    auto dy2 = dy * 2;
    auto e = 2 * dy - dx;
    auto current_y = p0.second;
    for (auto x0i = p0.first, x1i = p1.first; x0i != x1i; ++x0i) {
        e += dy2;
        if (e >= dx) {
            ++current_y;
            e-=dx2;
        }
        arr[(WIDTH * current_y) + x0i] = true;
    }
    return arr;
}

int main() {
    constexpr auto res = draw_line<HEIGHT, WIDTH>({2,2}, {40,20});
    print<HEIGHT, WIDTH>(res);
}
