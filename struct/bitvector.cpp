#include <iostream>
#include <algorithm>
#include <cassert>
#include <limits>

class BitVector {
    size_t get_size(size_t n) {
        double d_val = double(n)/double(CHAR_BIT);
        if (d_val > static_cast<double>(static_cast<size_t>(d_val))) {
            return static_cast<size_t>(d_val) + 1;
        } else {
            return static_cast<size_t>(d_val);
        }
    }

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
    class RBitIter;

    class BitRef {
        size_t index;
        BitVector& ba;
    public:
        friend BitIter;
        friend RBitIter;
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

    // reverse iterator
    // todo: optimize
    class RBitIter {
        bool last_index = false;
        BitRef br;
    public:       
        RBitIter(BitRef&& br): br(br){}
        RBitIter& operator++() {
            br.index == 0 ? (br = BitRef(--br.index, br.ba)) : (last_index = true);
            return *this;
        }
        RBitIter operator++(int) {
            auto tmp = *this;
            this->operator++();
            return tmp;
        }
        bool operator==(const RBitIter& rhs) {
             return br.index == rhs.br.index;
        }
        bool operator!=(const RBitIter& rhs) {
            return br.index != rhs.br.index && last_index == rhs.last_index;
        }
        BitRef& operator*() {
            return br;
        }
        operator bool() {
            return br.operator bool();
        }
    };
public:
    // todo: fix memory alloc
    BitVector(): ptr(new unsigned char[get_size(CHAR_BIT)]), cap(CHAR_BIT), sz(0){}
    BitVector(const std::initializer_list<bool>& il): ptr(new unsigned char[get_size(il.size())]), cap(get_size(il.size())*CHAR_BIT), sz(0) {
        for (auto el: il) {
            push_back(el);
        }
    }
    BitVector(const BitVector& rhs): ptr(new unsigned char[get_size(rhs.cap)]), cap(rhs.cap), sz(rhs.sz){
        std::copy(rhs.ptr, rhs.ptr+(get_size(sz)), ptr);
    }
    BitVector(const BitVector& rhs, size_t cap): ptr(new unsigned char[get_size(cap)]), cap(cap), sz(rhs.sz){
        std::copy(rhs.ptr, rhs.ptr+(get_size(sz)), ptr);
    }
    BitVector(BitVector&& rhs) noexcept: ptr(rhs.ptr), cap(rhs.cap), sz(rhs.sz) {
        rhs.ptr = nullptr;
        rhs.cap = 0;
        rhs.sz = 0;
    }

    BitVector& operator=(const BitVector& rhs) {
        if (this == &rhs) {
            return *this;
        }
        BitVector tmp(rhs);
        tmp.swap(*this);
        return *this;
    };

    BitVector& operator=(BitVector&& rhs){
        if (this == &rhs) {
            return *this;
        }
        rhs.swap(*this);
        return *this;
    };
    
    BitRef operator[](size_t index) {
        return BitRef(index, *this);
    }

    void swap(BitVector& rhs) {
        std::swap(rhs.ptr, ptr);
        std::swap(rhs.cap, cap);
        std::swap(rhs.sz, sz);
    }

    void push_back(bool value) {
        if (cap == sz) {
            BitVector tmp(*this, cap*2);
            tmp.swap(*this);
        }
        set(sz, value);
        ++sz;
    }
    
    BitIter begin() {
        return BitIter(BitRef(0, *this));
    }
    BitIter end() {
        return BitIter(BitRef(sz, *this));
    }

    size_t size() const {
        return sz;
    }

    size_t capacity() const {
        return cap;
    }

    RBitIter rbegin() {
        return RBitIter(BitRef(sz-1, *this));
    }

    RBitIter rend() {
        return RBitIter(BitRef(0, *this));
    }

    ~BitVector() {
        delete[] ptr;
    }
};


void* operator new(size_t size) {
    std::cout << "Alloc size:" << size << std::endl;
    return malloc(size);
}

int main() {
    BitVector bv1{1,0,1,1,1,1};
    std::vector<bool> bvec1 = {1,1,1,1,0,1};
    auto bvec1_index = 0;
    std::for_each(bv1.rbegin(), bv1.rend(), [&bvec1, &bvec1_index](const auto& el){
        assert(bvec1[bvec1_index++] == el);
    });

    BitVector bv{1,0,1,0,1,1};
    bv.push_back(1);
    assert(bv[bv.size()-1] == 1);

    bv.push_back(0);
    assert(bv[bv.size()-1] == 0);

    std::array<bool, 8> sarr = {1,0,1,0,1,1,1,0};
    size_t counter = 0;
    std::for_each(bv.begin(), bv.end(), [&sarr, &counter](const auto& el){
        assert(el == sarr[counter++]);
    });
    
    //
    BitVector bv2{1,0,1};
    BitVector tmp{0,0,0,0};
    bv2 = tmp;
    std::for_each(bv2.begin(), bv2.end(), [](const auto& el){
        assert(el == 0);
    });

    BitVector bv3{1,0,1};
    bv3 = BitVector{1,1,1,1};
    std::for_each(bv3.begin(), bv3.end(), [](const auto& el){
        assert(el == 1);
    });
}