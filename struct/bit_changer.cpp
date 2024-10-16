#include <iostream>

class BitChanger{
public:    
    char x{0};
    void set(size_t index, bool value) {
        if (value) {
            x |= (1 << index);
        } else {
            x &= ~(1 << index);
        }
    }
    std::string get_string() {
        return std::bitset<8>(x).to_string();
    }
};

int main() {
    BitChanger b;
    b.set(4, true);
    std::cout << b.get_string() << std::endl;
    b.set(1, true);
    std::cout << b.get_string() << std::endl;
    b.set(1, false);
    std::cout << b.get_string() << std::endl;
    b.set(4, false);
    std::cout << b.get_string() << std::endl;
}