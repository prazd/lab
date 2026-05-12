#include <iostream>
#include <bitset>
#include <cassert>

template <size_t n>
struct HammingCode {
    static consteval size_t bits_count() {
        size_t count = 0;
        for (size_t i = 1, current = n; current; ++i){
            if (!(i & (i - 1))){++count;continue;}
            ++count;--current;
        }
        return count;
    }
    static consteval size_t parity_bits_count() {
        size_t count = 0;
        for (size_t i = 1; i != bits_count(); ++i)
            count += (!(i & (i - 1)));
        return count;
    }

    std::bitset<bits_count()> bs;

    constexpr HammingCode() = default;
    constexpr HammingCode(size_t value) {
        for (int i = 0; i < n; ++i){
            set(i, value & 1);
            value >>= 1;
        }
    }

    constexpr void set(size_t index, bool value){
        for (int i = 1, current = index; i < bits_count() + 1; ++i){
            if (!(i & (i - 1))) continue;
            if (!current){bs[i-1] = value; break;}else{--current;}
        }
        for (size_t i = 0; i < parity_bits_count(); ++i){
            size_t val = 0;
            for (size_t j = 1; j < bits_count() + 1; ++j){
                if (!(j & (j - 1))) continue;
                if (j & (1 << i)) val ^= bs[j-1];
            }
            bs[(1 << i)-1] = val;
        }
    }

    constexpr void validate_and_flip() {
        size_t syndrome = 0;
        for (size_t i = 0; i < parity_bits_count(); ++i){
            size_t val = 0;
            for (size_t j = 1; j < bits_count() + 1; ++j){
                if (j & (1 << i)) val ^= bs[j-1];
            }
            syndrome |= (val << i);
        }
        if (syndrome) bs[syndrome-1] = !bs[syndrome-1];
    }
};


consteval void test(){

// [ 1  1  0 [0] 1 [1][0]]
//   7  6  5  4  3  2  1

// [1] = [3](1) ^ [5](0) ^ [7](1) -> 0
// [2] = [3](1) ^ [6](1) ^ [7](1) -> 1
// [4] = [5](0) ^ [6](1) ^ [7](1) -> 0

    HammingCode<4> hc(0b1101);
    assert(std::bitset<7>{0b1100110} == hc.bs);


    hc.bs[5] = 0;
    assert(std::bitset<7>{0b1000110} == hc.bs);

// [ 1 <0>  0 [0] 1 [1][0]] 
//   7  6   5  4  3  2  1

// [1](0) ^ [3](1) ^ [5](0) ^ [7](1) -> 0        =>   0
// [2](1) ^ [3](1) ^ [6](0) ^ [7](1) -> 1 != 0   =>   1   => 0b110 -> 6 pos error
// [4](0) ^ [5](0) ^ [6](0) ^ [7](1) -> 1 != 0   =>   1  

    hc.validate_and_flip();
    assert(std::bitset<7>{0b1100110} == hc.bs);

    hc.bs[2] = 0;
    assert(std::bitset<7>{0b1100010} == hc.bs);
    hc.validate_and_flip();
    assert(std::bitset<7>{0b1100110} == hc.bs);

    hc.validate_and_flip();
    assert(std::bitset<7>{0b1100110} == hc.bs);

    hc.bs[0] = 1;
    assert(std::bitset<7>{0b1100111} == hc.bs);
    hc.validate_and_flip();
    assert(std::bitset<7>{0b1100110} == hc.bs);
}

int main(){
    test();
}