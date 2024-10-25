#include <iostream>

class BitVector {
    static inline size_t default_sz = 10;
    unsigned char* ptr;
    size_t cap;
    size_t sz;
public:        
    BitVector(): ptr(new unsigned char[default_sz]), cap(0), sz(0){}
    BitVector(const BitVector&) = delete;
    BitVector(BitVector&&) = delete;
    BitVector& operator=(const BitVector&) = delete;
    BitVector& operator=(BitVector&&) = delete;
    ~BitVector() {
        delete[] ptr;
    }
};


int main() {
    BitVector bv;
}