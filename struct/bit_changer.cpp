#include <iostream>

class BitChanger{
public:    
    char x{0};
    void set_one(size_t index) {
        char tmp = 1;
        tmp<<=index;
        x|=tmp;
    }
    std::string get_string() {
        return std::bitset<8>(x).to_string();
    }
};

int main() {
    BitChanger b;
    b.set_one(7);
    std::cout << b.get_string() << std::endl;
    b.set_one(0);
    std::cout << b.get_string() << std::endl;
}