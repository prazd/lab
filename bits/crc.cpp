#include <iostream>
#include <climits>

constexpr size_t CRC16_ARC = 0x18005;

consteval size_t msb(unsigned long msg){
    size_t r = 0;
    for (size_t i = 0, e = sizeof(msg) * CHAR_BIT; i != e; ++i){
        r = (msg & 1) ? i : r;
        msg >>= 1;
    }
    return r;
}

consteval unsigned long crc_gen(unsigned long msg, unsigned long p){
    auto p_msb = msb(p);
    auto c = msb(msg) + 1ul;
    msg <<= p_msb;
    auto d_msg = msg;
    auto s = p_msb + c;
    auto shift = s - (p_msb + 1ul);
    auto d = p << shift;
    size_t pos = 1ul << (s - 1ul);
    for (size_t i = 0; i != c; ++i){
        d_msg ^= (d_msg & pos) ? d : 0;
        pos >>= 1ul;
        d >>= 1ul;
    }
    return msg | d_msg;
}

consteval bool crc_ok(unsigned long msg, unsigned long p){
    auto m_msb = msb(msg);
    auto p_msb = msb(p);
    auto shift = m_msb - p_msb;
    auto c = shift + 1;
    auto d = p << shift;
    auto pos = 1ul << (m_msb);
    auto d_msg = msg;
    for (size_t i = 0; i != c; ++i){
        d_msg ^= (d_msg & pos) ? d : 0;
        pos >>= 1ul;
        d >>= 1ul;
    }
    return d_msg == 0;
}


int main(){
    static_assert(crc_gen(0b1101, 0b1011) == 0b1101001);
    static_assert(crc_ok(0b1101001, 0b1011));
    static_assert(!(crc_ok(0b1001001, 0b1011)));
    
    static_assert(crc_gen(0b1101, CRC16_ARC) == 0b11011000000000101101);
    static_assert(crc_ok(0b11011000000000101101, CRC16_ARC));
    static_assert(!(crc_ok(0b11001000000000101101, CRC16_ARC)));
    
    static_assert((crc_ok(
        0b11011000000000101101 ^ (CRC16_ARC << 3), 
        CRC16_ARC))); // false positive
}