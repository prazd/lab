#include <iostream>
#include <fstream>

constexpr const char* filename = "mario.jpg";

template <typename Iter>
void print_bytes(Iter it, Iter ite) {
    std::cout << std::hex;
    size_t counter = 0;
    std::cout << "{";
    for (;it!=ite; ++it) {
        std::cout << "0x" << static_cast<int>(static_cast<uint8_t>(*it));
        ++counter;
        if (std::next(it) != ite) {
            std::cout << ",";
        }
    }
    std::cout << " }" << std::dec << std::endl;
}

template <typename Iter>
Iter find_byte(Iter begin, char v1, char v2) {
    for (;*begin != v1 || *(begin + 1) != v2; ++begin) {}
    return begin;
}

int main() {
    std::ifstream is(filename, std::ios::binary);
    if (!is.is_open()) {
        std::abort();
    }
    const size_t file_size = static_cast<size_t>(std::filesystem::file_size(filename));
    std::vector<char> buffer(file_size);
    is.read(buffer.data(), file_size);
    auto it = find_byte(buffer.begin(), 0xff, 0xc4);
    auto ite = find_byte(it+2, 0xff, 0xc4);
    print_bytes(it, ite+2);
}