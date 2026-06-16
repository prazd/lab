#ifndef NES_APU_H
#define NES_APU_H
#include "Bus.hpp"
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

template <unsigned short timer_l,
          unsigned short timer_h,
          unsigned short duty_envelope>
struct PulseChan : Channel {
    unsigned char current_duty_step{};
    inline void tick_timer(Bus& bus){
        --timer;
        if (!timer){
            current_duty_step = (current_duty_step + 1) % 8;
            update_timer(bus.unsafe_read_prg(timer_l), bus.unsafe_read_prg(timer_h));
        }
    }
    inline unsigned char get_duty_cycle(Bus& bus) const {
        switch((bus.unsafe_read_prg(duty_envelope) >> 6) & 0x3){
            case 0:  return  0b0100'0000; // 12.5%
            case 1:  return  0b0110'0000; // 25%
            case 2:  return  0b0111'1000; // 50%
            default: return  0b1001'1111; // 25% (negated)
        }
    }
    inline unsigned char get_duty_value(Bus& bus) const {
        return (get_duty_cycle(bus) >> current_duty_step) & 1;
    }
    inline unsigned char get_dac(Bus& bus) const {
        return get_duty_value(bus) ? envelope & 0xf : 0;
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
    short buffer[1024]{};
    size_t buffer_i = 0;
    size_t current_tick = 0;
    size_t sample_acc = 0;

    PulseChan<constants::PULSE1_TIMER_LOW,
            constants::PULSE1_TIMER_HIGH,
            constants::PULSE1_DUTY_ENVELOPE> pulse_ch1{};

    PulseChan<constants::PULSE2_TIMER_LOW,
            constants::PULSE2_TIMER_HIGH,
            constants::PULSE2_DUTY_ENVELOPE> pulse_ch2{};

    TriangleChan triangle_ch{};
    NoiseChan noise_ch{};
    DMCChan dmc_ch{};

    size_t cycle_counter{};
    bool irq_inhibit = false;

    enum FrameCounterMode {
        Step4, Step5
    } frame_counter_mode = Step4;

    void tick(Bus& bus);
    void handle_frame_counter_update(Bus& bus);
    void frame_counter_step4(Bus& bus);
    void frame_counter_step5(Bus& bus);
    void frame_counter_quarter_update(Bus& bus);
    void frame_counter_half_update(Bus& bus);
    float gen_sample(Bus& bus);
};
#endif
