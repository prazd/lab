#include <iostream>
#include <vector>
#include <bit>

template <typename T>
class BitChanger{
    unsigned char* get_ptr(size_t index) {
        auto iter = vec.begin();
        auto r = iter->range;
        while(index >= r) {
            ++iter;
            r = iter->range;
        }
        return iter->ptr;
    }

    struct Data {
        unsigned char* ptr;
        size_t range;
    };

    std::vector<Data> vec;
    
public:
    BitChanger(T& val) {
        size_t r = 0;
        auto val_ptr = reinterpret_cast<unsigned char*>(&val);
        for(unsigned long i = 0;i < sizeof(val); ++i) {
            r+=CHAR_BIT;
            vec.push_back(Data{.ptr = val_ptr+i, .range = r});
        }
    }
    void set(size_t index, bool value) {
        auto x = get_ptr(index);
        index %= CHAR_BIT;
        if (value) {
            *x |= (1 << index);
        } else {
            *x &= ~(1 << index);
        }
    }

    std::string get_string() {
        std::string res;
        for(auto el: vec) {
            res += std::bitset<8>(*(el.ptr)).to_string();
        }
        return res;
    }
};

struct Base {
    int x{1024};
    int y{0};
};

int main() {
    Base b;
    BitChanger<Base> x(b);
    std::cout << x.get_string() << std::endl;
    x.set(10,0);
    x.set(0,1);
    x.set(1,1);
    x.set(2,1);
    x.set(3,1);
    x.set(4,1);
    x.set(5,1);
    x.set(6,1);
    x.set(7,1);
    x.set(8,1);
    x.set(9,1);
    x.set(10,1);
    x.set(11,1);
    x.set(12,1);
    x.set(13,1);
    x.set(14,1);
    x.set(15,1);
    std::cout << x.get_string() << std::endl;
    x.set(15,0);
    std::cout << x.get_string() << std::endl;
    std::cout << b.x << std::endl;
}