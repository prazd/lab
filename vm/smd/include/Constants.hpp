#ifndef SMD_CONSTANTS_HPP
#define SMD_CONSTANTS_HPP
#define DEBUG_MODE 0

#if !DEBUG_MODE
#define S_MODE static
#else
#define S_MODE
#endif

namespace constants {
    constexpr size_t MC68K_CPU_CYCLE = 7'670'000;
    constexpr size_t Z80_CPU_CYCLE = 3'580'000;
};
#endif