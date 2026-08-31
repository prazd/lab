#ifndef SMD_BUS_HPP
#define SMD_BUS_HPP
#include "Constants.hpp"
struct BUS {
    S_MODE unsigned char mc68kmem[1 << 24]{};
    S_MODE unsigned char z80mem[1 << 16]{};
};
#endif