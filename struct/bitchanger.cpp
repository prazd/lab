#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <limits>
#include <bit>

template <typename T>
class BitChanger{
    unsigned char* buffer[sizeof(T)];

    unsigned char* get_ptr(size_t index) & {
        return buffer[index / CHAR_BIT];
    }

    const unsigned char* get_ptr(size_t index) const& {
       return buffer[index / CHAR_BIT];
    }

    void set(size_t index, bool value) & {
        auto x = get_ptr(index);
        index %= CHAR_BIT;
        if (value) {
            *x |= (1 << index);
        } else {
            *x &= ~(1 << index);
        }
    }

    bool get(size_t index) const& {
        auto x = get_ptr(index);
        index %= CHAR_BIT;
        unsigned char res = 1 << index;
        return res & *x;
    }

    class BitIter;

    class BitRef {
        size_t index;
        BitChanger& bc;
        BitRef& operator++() & {
            ++index;
            return *this;
        }
    public:
        friend BitIter;
        BitRef(size_t index, BitChanger& bc): bc{bc}, index{index}{}
        BitRef& operator=(bool value) & {
            bc.set(index, value);
            return *this;
        }
        BitRef(const BitRef& rhs): index(rhs.index), bc(rhs.bc){}
        BitRef& operator=(const BitRef& rhs) & {
            bc.set(static_cast<bool>(rhs));
            return *this;
        }
        BitRef& operator=(BitRef&& rhs) & {
            std::swap(*this, rhs);
            return *this;
        }
        operator bool() const {
            return bc.get(index);
        }
        operator std::string() const {
            return this->operator bool() ? "1" : "0";
        }
    };

    class BitIter{
        BitRef br;
    public:        
        BitIter(BitRef&& br): br{br} {}
        BitIter& operator++() {
            ++br;
            return *this;
        }
        BitIter operator++(int) {
            auto tmp = *this;
            this->operator++();
            return tmp;
        }
        bool operator==(const BitIter& bi) {
            return br.index == bi.br.index;
        }
        bool operator!=(const BitIter& bi) {
            return !(*this == bi);
        }
        BitRef& operator*() {
            return br;
        }
    };
  
public:
    BitChanger(T& val) {
        size_t r = 0;
        auto val_ptr = reinterpret_cast<unsigned char*>(&val);
        for(unsigned long i = 0;i < sizeof(val); ++i) {
            r+=CHAR_BIT;
            buffer[i] = val_ptr+i;
        }
    }

    BitRef operator[](size_t index) {
        return BitRef(*this, index);
    }

    std::string get_string() {
        std::string res;
        for(auto begin = std::begin(buffer), end = std::end(buffer);begin!=end;++begin) {
            auto tmp = std::bitset<8>(**begin).to_string();
            std::reverse(tmp.begin(), tmp.end());
            res += tmp;
        }
        return res;
    }
    BitIter begin() & {
        return BitIter(BitRef(0, *this));
    }
    BitIter end() & {
        return BitIter(BitRef(sizeof(T)*CHAR_BIT, *this));
    }
    const BitIter cbegin() const& {
        return BitIter(BitRef(0, *this));
    }
    const BitIter cend() const& {
        return BitIter(BitRef(sizeof(T)*CHAR_BIT, *this));
    }
};

struct TestStruct {
    unsigned x{567};
    unsigned y{423};
};

int main() {
    TestStruct t;
    BitChanger<TestStruct> bc(t);

    std::cout << "BitChanger size:" << sizeof(bc) << std::endl;
    assert(sizeof(TestStruct)*(CHAR_BIT) == sizeof(bc));
    auto bc_string = bc.get_string();
    std::string res;
    std::for_each(std::begin(bc), std::end(bc), [&res](const auto& val){res += (std::string)val;});
    std::cout << bc_string << std::endl;
    std::cout << res << std::endl;
    assert(std::equal(res.begin(), res.end(), bc_string.begin(), bc_string.end()));

    std::for_each(std::begin(bc), std::end(bc), [](auto& val){val = 0;});
    assert((t.x == 0) && (t.y == 0));
    std::cout << "Zero check:" << bc.get_string() << std::endl;

    std::for_each(std::begin(bc), std::end(bc), [](auto& val){val = 1;});
    assert((t.x == std::numeric_limits<unsigned>::max()) && (t.y == std::numeric_limits<unsigned>::max()));
    std::cout << "Max uint32 check:" << bc.get_string() << std::endl;
    
}