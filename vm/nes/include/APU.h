#ifndef NES_APU_H
#define NES_APU_H
#include "Bus.hpp"
#include "CPU.hpp"
struct Channel {
    unsigned short timer{};
    unsigned char envelope{};
    unsigned char length_counter{};
    unsigned char sweep_shift{};

    inline void update_timer(unsigned char l, unsigned short h){
        timer = (((h & 0x7) << 8) | l);
    }
    inline void update_envelope(unsigned char val){
        envelope = (val & 0xf);
    }
    inline void update_length_counter(unsigned char val) {
        length_counter = (val >> 3) & 0x1f;
    }
};

struct PulseChan : Channel {
    unsigned char current_duty_step{};
    inline unsigned char get_duty_cycle(unsigned char val){
        switch((val >> 6) & 0x3){
            case 0:  return  0b0100'0000; // 12.5%
            case 1:  return  0b0110'0000; // 25%
            case 2:  return  0b0111'1000; // 50%
            default: return  0b1001'1111; // 25% (negated)
        }
    }
};

struct NoiseChan : Channel {
    // todo
};

struct TriangleChan : Channel{
    // todo
};

struct DMCChan : Channel {
    // todo
};

struct APU {
    PulseChan pulse_ch1{};
    PulseChan pulse_ch2{};
    TriangleChan triangle_ch{};
    NoiseChan noise_ch{};
    DMCChan dmc_ch{};

    size_t cycle_counter{};
    bool irq_inhibit = false;

    enum FrameCounterMode {
        Step4, Step5
    } frame_counter_mode = Step4;

    void tick(Bus& bus, CPU& cpu);
    void handle_frame_counter_update(Bus& bus, CPU& cpu);
    void frame_counter_step4(Bus& bus, CPU& cpu);
    void frame_counter_step5(Bus& bus, CPU& cpu);
    void frame_counter_quarter_update(Bus& bus, CPU& cpu);
    void frame_counter_half_update(Bus& bus, CPU& cpu);
};
#endif
