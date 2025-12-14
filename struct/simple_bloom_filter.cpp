#include <iostream>
#include <bitset>
#include <climits>
#include <cassert>

template <typename T, size_t n = 1024, size_t k = 3>
class SimpleBloomFilter {
    std::hash<T> hf;
    unsigned char buffer[n/CHAR_BIT]{};

    // MurmurHash3
    size_t mix(size_t val) {
        val ^= val >> 33;
        val *= 0xff51afd7ed558ccdULL;
        val ^= val >> 33;
        val *= 0xc4ceb9fe1a85ec53ULL;
        val ^= val >> 33;
        return val;
    }
public:
    void push(T value){
        auto h = hf(value);
        auto mh = mix(h);
        for (size_t i = 0; i < k; ++i) {
            auto r = (h + i * mh) % n;
            auto byte_n = r/8;
            buffer[byte_n] |= (1 << (r%8));
        }
    }
    bool is_in_set(T value) {
        auto h = hf(value);
        auto mh = mix(h);
        for (size_t i = 0; i < k; ++i) {
            auto r = (h + i * mh) % n;
            auto byte_n = r/8;
            if (!((buffer[byte_n] >> (r%8)) & 1)){
                return false;
            }
        }
        return true;
    }
};

int main() {
    SimpleBloomFilter<int> bf;
    bf.push(30);
    assert(bf.is_in_set(30));
    assert(!(bf.is_in_set(20)));
}