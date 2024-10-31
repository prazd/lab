#include <iostream>
#include <limits>
#include <climits>

template <typename T>
size_t count_one(T& val) {
    size_t count = 0;
    auto ptr = reinterpret_cast<unsigned char*>(&val);
    for(int i = 0; i < sizeof(val)*CHAR_BIT; ++i) {
        ptr[i/CHAR_BIT] & (1 << i%CHAR_BIT) ? ++count : count;
    }
    return count;
}

template <typename T>
constexpr size_t count_one_comptime(T& val) {
    unsigned int arr[sizeof(val)];
    for(int i = 0; i < sizeof(val);++i) {
        arr[i] = (val >> i*CHAR_BIT) & 0xFF;
    }
    size_t count = 0;
    for(int i = 0; i < sizeof(val)*CHAR_BIT; ++i) {
        arr[i/CHAR_BIT] & (1 << i%CHAR_BIT) ? ++count : count;
    }
    return count;
}

int main() {
    constexpr unsigned int x = std::numeric_limits<unsigned int>::max();
    constexpr auto result = count_one_comptime(x);
    std::cout << result << "\n";

    unsigned int y = std::numeric_limits<unsigned int>::max();;
    auto result2 = count_one(y);
    std::cout << result2;
}