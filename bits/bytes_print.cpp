#include <iostream>
#include <fstream>

constexpr const char* filename = "a.out";
const size_t file_size = static_cast<size_t>(std::filesystem::file_size(filename));

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

int main() {
    std::ifstream is(filename, std::ios::binary);
    if (!is.is_open()) {
        std::abort();
    }
    std::vector<char> buffer(file_size);
    is.read(buffer.data(), file_size);
    print_bytes(buffer.begin(), buffer.end());
}