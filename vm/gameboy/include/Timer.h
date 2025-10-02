#ifndef GAMEBOY_TIMER_H
#define GAMEBOY_TIMER_H
#include "Memory.h"
struct Timer {
    unsigned short div_internal = 0;
    void tick(Memory&);
};
#endif
