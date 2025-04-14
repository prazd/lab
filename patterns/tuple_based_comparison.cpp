#include <iostream>

struct Base {
    int x;
    int y;
    constexpr bool operator<(const Base& rhs) {
        return std::tie(x, y) < std::tie(rhs.x, rhs.y);
    }
};

int main() {
    static_assert(Base{10,20} < Base{30,40});
}