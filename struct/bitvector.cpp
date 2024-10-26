#include <iostream>
#include <algorithm>

class BitVector {
    static inline size_t default_sz = 2;

    unsigned char* ptr;
    size_t cap;
    size_t sz;

    unsigned char* get_ptr(size_t index) {
        return &ptr[index / CHAR_BIT];
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

    class BitIter; // decl for friend
    class BitRef {
        size_t index;
        BitVector& ba;
    public:
        friend BitIter;
        BitRef(size_t index, BitVector& ba): index(index), ba(ba){}
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
    };

    class BitIter {
        BitRef br;
    public:        
        BitIter(BitRef&& br): br(br) {}
        BitIter& operator++() {
            br = BitRef(++br.index, br.ba);
            return *this;
        }
        BitIter operator++(int) {
            auto tmp = *this;
            this->operator++();
            return tmp;
        }
        bool operator==(const BitIter& rhs) {
            return br.index == rhs.br.index;
        }
        bool operator!=(const BitIter& rhs) {
            return br.index != rhs.br.index;
        }
        BitRef& operator*() {
            return br;
        }
        operator bool() {
            return br.operator bool();
        }
    };

public:        
    BitVector(): ptr(new unsigned char[default_sz]), cap(0), sz(0){}
    BitVector(const BitVector& rhs): ptr(new unsigned char[rhs.cap]), cap(rhs.cap), sz(rhs.sz){
        std::copy(rhs.ptr, rhs.ptr+sz, ptr);
    }
    BitVector(const BitVector& rhs, size_t cap): ptr(new unsigned char[cap]), cap(cap), sz(rhs.sz){
        std::copy(rhs.ptr, rhs.ptr+sz, ptr);
    }
    BitVector(BitVector&& rhs): ptr(rhs.ptr), cap(rhs.cap), sz(rhs.sz){}
    
    BitRef operator[](size_t index) {
        return BitRef(index, *this);
    }

    void swap(BitVector& rhs) {
        std::swap(rhs.ptr, ptr);
        std::swap(rhs.cap, cap);
        std::swap(rhs.sz, sz);
    }

    void push_back(bool val) {
        if (cap == sz) {
            BitVector tmp(*this, cap*2);
            tmp.swap(*this);
        }
        auto br = this->operator[](sz);
        br = val;
        ++sz;
    }
    
    BitVector& operator=(const BitVector&) = delete;
    BitVector& operator=(BitVector&&) = delete;
    
    BitIter begin() {
        return BitIter(BitRef(0, *this));
    }
    BitIter end() {
        return BitIter(BitRef(sz, *this));
    }
    ~BitVector() {
        delete[] ptr;
    }

};


int main() {
    BitVector bv;
    // todo: initializer list + 
    bv.push_back(1);
    bv.push_back(0);
    bv.push_back(1);
    bv.push_back(0);
    bv.push_back(1);
    bv.push_back(1);
    std::for_each(bv.begin(), bv.end(), [](const auto& el){
        std::cout << el << std::endl;
    });
}