#ifndef MAIN_CPP_APU_H
#define MAIN_CPP_APU_H
#include "Memory.h"

struct Channel {
    unsigned short freq_timer{};
    unsigned char length_counter{};
    bool active{};
};

struct CH1: Channel {
    unsigned short envelope_timer{};
    unsigned short sweep_timer{};
    float duty{};
    void update_duty(Memory& mem);
    void update_length_timer(Memory& mem);
};

struct CH2: Channel {
    unsigned short envelope_timer{};
    unsigned char duty{};
    void update_length_timer(Memory& mem);
};

struct CH3: Channel {
    unsigned char wave{};
    unsigned char volume{};
    void update_length_timer(Memory& mem);
};

struct CH4: Channel {
    unsigned char lsfr{};
    unsigned short envelope_timer{};
    void update_length_timer(Memory& mem);
};

struct FrameSequencer {
    unsigned char step{};
    unsigned short frame_seq_counter{};
    void tick();
};

struct APU {
    CH1 ch1;
    CH2 ch2;
    CH3 ch3;
    CH4 ch4;

    void tick(Memory&);
};
#endif //MAIN_CPP_APU_H
