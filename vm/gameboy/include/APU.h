#ifndef MAIN_CPP_APU_H
#define MAIN_CPP_APU_H
#include "Memory.h"

struct Channel {
    unsigned short freq_timer{};
    unsigned short length_counter{};
    unsigned short envelope_timer{};
    unsigned short sweep_timer{};
    unsigned char phase_counter{};
    bool active{};
};

struct APU {
    Channel ch1;
    Channel ch2;
    Channel ch3;
    Channel ch4;

    void tick(Memory&);
};
#endif //MAIN_CPP_APU_H
