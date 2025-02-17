#include <iostream>
#include <array>
#include <numeric>

template <typename T, size_t ROW, size_t COL>
struct MortonOrder {
    static inline T buffer[ROW][COL]{};

    static constexpr size_t morton_index(size_t x, size_t y) {
        size_t morton = 0;
        for (size_t i = 0, e = sizeof(size_t) * 4; i < e; ++i) {
            morton |= ((x >> i & 1) << (2 * i)) | ((y >> i & 1) << (2 * i + 1));
        }
        return morton;
    }

    template <typename Container>
    MortonOrder(const Container& ct) {
        for (size_t y = 0; y < ROW; ++y) {
            for (size_t x = 0; x < COL; ++x) {
                size_t morton = morton_index(x, y);
                buffer[y][x] = ct[morton];
            }
        }
    }

    void print() {
        for (auto&& r: buffer) {
            for (auto el: r) {
                std::cout << el << " ";
            }
            std::cout << "\n";
        }
    }
};

int main() {
    std::array<int, 16 * 16> arr;
    std::iota(arr.begin(), arr.end(), 1);
    MortonOrder<int, 16, 16> m{arr};
    m.print();
}
