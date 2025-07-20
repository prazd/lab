#ifndef GAMEBOY_TIMER_H
#define GAMEBOY_TIMER_H
#include "RAM.h"
struct Timer {
    unsigned short div_internal = 0;
    void handle(int t_cycles, RAM&);
};
#endif
