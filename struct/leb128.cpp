#include <iostream>
#include <cassert>
#include <climits>
#include <limits>

template <typename T, std::forward_iterator It>
constexpr auto decode_leb128(It it){
    T res{};
    size_t pos{};
    size_t max_pos = sizeof(T) * CHAR_BIT;
    for (;;){
        auto val = *it & 0x7f;
        res |= (val << pos);
        pos += 7;
        if (pos >= max_pos){
            pos = (sizeof(T) * CHAR_BIT) - 1;
            break;
        }
        if (!(*it & 0x80)) break;
        ++it;
    }
    if constexpr (std::is_signed_v<T>) {
        if (*it & 0x40 && pos != (sizeof(T) * CHAR_BIT) - 1) res |= (~(T(((1 << pos)) - 1)));
    }
    return res;
}

consteval void test(){
    // ULEB
    unsigned char arr0[1] = {0x7f};
    assert(decode_leb128<unsigned>(arr0) == 0x7f);
    unsigned char arr1[2] = {0x80,0x1};
    assert(decode_leb128<unsigned>(arr1) == 128);
    unsigned char arr2[2] = {0xe8,0x07};
    assert(decode_leb128<unsigned>(arr2) == 1000);
    unsigned char arr3[5] = {0xff,0xff,0xff,0xff,0xf};
    assert(decode_leb128<unsigned>(arr3) == std::numeric_limits<unsigned>::max());

    // SLEB
    constexpr unsigned char arr4[1] = {0x40};
    assert(decode_leb128<int>(arr4) == -64);
    constexpr unsigned char arr5[2] = {0x80,0x7f};
    assert(decode_leb128<int>(arr5) == -128);
    constexpr unsigned char arr6[5] = {0x80,0x80,0x80,0x80,0x78};
    assert(decode_leb128<int>(arr6) == std::numeric_limits<int>::min());
    constexpr unsigned char arr7[6] = {0xff,0xff,0xff,0xff,0x7};
    assert(decode_leb128<int>(arr7) == std::numeric_limits<int>::max());
    
}

int main(){
    test();
}