#ifndef GAMEBOY_TIMER_H
#define GAMEBOY_TIMER_H
#include "Memory.h"
struct Timer {
    unsigned short div_internal = 0;
    void tick(Memory&);

    unsigned char prev_bit = 0;
    unsigned char cur_bit = 0;
    bool tima_overflow = false;
    unsigned short prev_div = 0;
    unsigned char tac_prev = 0xf8;
    int overflow_counter = 0;
    size_t current_t_cycle = 0;
};
#endif
