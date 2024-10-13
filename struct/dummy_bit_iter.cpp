#include <iostream>
#include <bit>

struct Base {
    char x{1};
    char y{2};
};

template <typename T>
class BitIter {
    size_t sz;
    std::string bit_values;
public:   
    BitIter(const T& val): sz(sizeof(T)*8) {
        auto ptr = reinterpret_cast<const unsigned char*>(&val);
        for (int i = 0;i<sizeof(val);++i) {
            std::bitset<8> b(ptr[i]);
            bit_values = b.to_string() + bit_values;
        }
        std::reverse(bit_values.begin(), bit_values.end());
    }
    const std::string& value() const& {
        return bit_values;
    }
    const char operator[](size_t index) {
        return bit_values[index];
    }
    const char* begin() const {
        return &bit_values[0];
    }
    const char* end() const {
        return &bit_values[bit_values.size()];
    }
    const char* cbegin() const {
        return &bit_values[0];
    }
    const char* cend() const {
        return &bit_values[bit_values.size()];
    }
};


int main() {
    Base b;
    BitIter<Base> bi(b);
    std::cout << bi.value() << std::endl;
    for(auto el: bi) {
        std::cout << el << std::endl;
    }
}