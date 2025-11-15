#ifndef MAIN_CPP_APU_H
#define MAIN_CPP_APU_H
#include "Memory.h"

struct Channel {
    unsigned short freq_timer{};
    unsigned short freq_timer_period{};
    unsigned char length_counter{};
    unsigned char phase{};
    bool active{};
};

struct SquareWaveChannel: Channel {
    float dac_out{};
    unsigned shadow_frequency{};
    unsigned short envelope_timer{};
    char envelope_volume{};
    unsigned short sweep_timer{};
    unsigned char duty{};
    void update_duty(const Memory& mem, unsigned short);
    void update_pitch(const Memory& mem, unsigned short l_addr, unsigned short h_addr);

};

struct CH1: SquareWaveChannel {
    void init(Memory&);
    void sweep_step(Memory& mem, bool);
    void disable_check(Memory& mem);
    void disable(Memory& mem);
};

struct CH2: SquareWaveChannel {
    void init(const Memory&);
    void disable_check(Memory& mem);
    void disable(Memory& mem);
};

struct CH3: Channel {
    bool length_enabled{};
    float dac_out{};
    unsigned short freq{};
    unsigned char wave{};
    unsigned char nibble{};
    void disable_check(Memory& mem);
    void disable(Memory& mem);
    void init(Memory&);
};

struct CH4: Channel {
    unsigned short envelope_timer{};
    unsigned char lsfr{};
    void update_length_timer(const Memory& mem);
};

struct APU {
    CH1 ch1;
    CH2 ch2;
    CH3 ch3;
    CH4 ch4;
    short buffer[constants::SAMPLE_BUFFER_SIZE]{};
    unsigned char frame_sequencer_step{};
    unsigned frame_seq_t_cycle{};
    void tick(Memory&);
    void frame_seq_tick(Memory&);
    void frame_seq_handler(Memory& mem, unsigned char step);
};
#endif //MAIN_CPP_APU_H
