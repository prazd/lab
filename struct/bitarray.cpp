#include <iostream>

template <size_t n>
class BitArray {
    static constexpr size_t get_size() {
        constexpr double d_val = double(n)/double(CHAR_BIT);
        if constexpr(d_val > static_cast<double>(static_cast<size_t>(d_val))) {
            return static_cast<size_t>(d_val) + 1;
        } else {
            return static_cast<size_t>(d_val);
        }
    }
    unsigned char buffer[get_size()];
    unsigned char* get_ptr(size_t index) {
        auto iter = std::begin(buffer);
        auto r = n/CHAR_BIT;
        while(index >= r) {
            ++iter;
            r+=n/CHAR_BIT;
        }
        return iter;
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

    bool operator==(const BitArray& rhs) {
        return this == &rhs;
    }
    
    struct BitRef {
        size_t index;
        BitArray& ba;
        BitRef(size_t index, BitArray& ba): index(index), ba(ba){}
        BitRef& operator=(bool value) {
            ba.set(index, value);
            return *this;
        }
        BitRef& operator=(BitRef& rhs) {
            BitRef tmp(rhs.index, rhs.ba);
            std::swap(*this, tmp);
            return *this;
        }
        operator bool() const {
            auto x = ba.get_ptr(index);
            auto l_index = index%CHAR_BIT;
            unsigned char res = (1 << l_index);
            return res & *x;
        }
        bool operator==(const BitRef& rhs) {
            return index == rhs.index;
        }
        bool operator!=(const BitRef& rhs) {
            return index != rhs.index;
        }
     };
    struct BitIter {
        BitRef br;
        BitIter(BitRef&& br): br(br) {}
        BitIter& operator++() {
            br = BitRef(++br.index, br.ba);
            return *this;
        }
        BitIter operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }
        bool operator==(const BitIter& rhs) {
            return br == rhs.br;
        }
        bool operator!=(const BitIter& rhs) {
            return br != rhs.br;
        }
        BitRef& operator*() {
            return br;
        }
        operator bool() {
            return br.operator bool();
        }
    };
public:
    BitArray(std::initializer_list<bool> list) {
        auto counter = 0;
        for(auto b = list.begin(), e = list.end();b!=e;++b) {
            set(counter++, *b);
        }
    }
    BitRef operator[](size_t index) {
        return BitRef(index, *this);;
    }
    BitIter begin() {
        auto res = BitIter(BitRef(0, *this));
        return res;
    }
    BitIter end() {
        auto res = BitIter(BitRef(n, *this));
        return res;
    }
    const BitIter cbegin() {
        auto res = BitIter(BitRef(0, *this));
        return res;
    }
    const BitIter cend() {
        auto res = BitIter(BitRef(n, *this));
        return res;
    }
};


int main() {
    BitArray<9> barr{1,1,0,0,1,1,1,1,1};
    std::cout << "Size:" << sizeof(barr) << std::endl;
    std::for_each(barr.begin(), barr.end(), [](auto& el){el=1;});
    std::for_each(barr.begin(), barr.end(), [](const auto& el){std::cout << el;});
    std::cout << "\n";
    std::for_each(barr.begin(), barr.end(), [](auto& el){el=0;});
    std::for_each(barr.begin(), barr.end(), [](const auto& el){std::cout << el;});
}