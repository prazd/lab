#ifndef MAIN_CPP_APU_H
#define MAIN_CPP_APU_H
#include "Memory.h"

struct Channel {
    bool freezed{};
    bool extra_clock{};
    unsigned short freq_timer{};
    unsigned short freq_timer_period{};
    unsigned short length_counter{};
    unsigned char phase{};
    bool active{};
};

struct SquareWaveChannel: Channel {
    unsigned dac_out{};
    unsigned shadow_frequency{};

    unsigned short envelope_timer{};
    char envelope_volume{};
    unsigned short sweep_timer{};
    unsigned char duty{};
    void update_duty(const Memory& mem, unsigned short);
    void update_pitch(const Memory& mem, unsigned short l_addr, unsigned short h_addr);
};

struct CH1: SquareWaveChannel {
    bool sweep_unit_freeze = true;
    bool delayed_off = false;
    bool sweep_trigger_disabled = false;
    bool fast_off = false;
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
    unsigned char delay = 2;
    unsigned char dac_out{};
    unsigned short freq{};
    unsigned char wave{};
    unsigned char nibble{};
    void disable_check(Memory& mem);
    void disable(Memory& mem);
    void init(Memory&);
};

struct CH4: Channel {
    unsigned char dividers[8]{
      8,16,32,48,64, 80, 96, 112
    };
    unsigned char envelope_volume{};
    unsigned short envelope_timer{};
    bool lsfr_width{};
    unsigned freq_timer{};
    unsigned freq_period{};
    unsigned dac_out{};
    unsigned short lsfr{};
    void init(const Memory&);
    void disable_check(Memory& mem);
    void disable(Memory& mem);
};

struct APU {
    CH1 ch1;
    CH2 ch2;
    CH3 ch3;
    CH4 ch4;
    unsigned t_cycle = 0;
    unsigned char prev_frame_seq_step = 0;
    bool skip_sample = false;
    unsigned short count_sample = 0;
    unsigned accum{};
    int current_buffer_i {};
    short buffer[constants::SAMPLE_BUFFER_SIZE]{};
    unsigned char frame_sequencer_step{};
    unsigned frame_seq_t_cycle{};
    void tick(Memory&);
    void frame_seq_tick(Memory&);
    void frame_seq_handler(Memory& mem, unsigned char step);
    std::pair<float, float> next_sample_2ch(Memory& mem);
};
#endif //MAIN_CPP_APU_H
