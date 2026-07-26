#ifndef NES_APU_HPP
#define NES_APU_HPP
#include "Bus.hpp"
#include "Simple.hpp"

struct Envelope {
    unsigned char prev_divider{};
    unsigned char divider{};
    unsigned char decay_lvl = 0;
    bool is_volume_const = false;
    unsigned char volume_const_val = 0;
    bool is_infinite_play = false;
    inline void update_envelope(Bus& bus, unsigned char val){
        is_volume_const = val & 0x10;
        volume_const_val = val & 0xf;
        is_infinite_play = val & 0x20;
        prev_divider = val & 0xf;
    }
    inline unsigned char get_volume(Bus& bus) const{
        return is_volume_const ? volume_const_val : decay_lvl;
    }
//    https://www.nesdev.org/wiki/APU_Envelope
    inline void envelope_tick(Bus& bus){
        // TODO: check
        if (divider) --divider;
        else if (!divider){
            divider = prev_divider;
            if (decay_lvl) --decay_lvl;
            else if (!decay_lvl && is_infinite_play) {
                decay_lvl = 15;
            }
        }
    }
};

struct Channel {
    unsigned short timer{};
    unsigned char length_counter{};
    inline void update_timer(unsigned char l, unsigned char h){
        timer = (((h & 0x7) << 8) | l);
    }
};


//  https://www.nesdev.org/wiki/APU_Pulse
template <unsigned short timer_l,
          unsigned short timer_h,
          unsigned short duty_envelope>
struct PulseChan : Channel, Envelope {
    bool sweep_enable = false;
    bool sweep_is_neg = false;
    bool sweep_reset = false;
    unsigned char sweep_period = 0;
    unsigned char sweep_shift = 0;
    unsigned char sweep_counter = 0;
    unsigned short sweep_delta = 0;
    unsigned short sweep_target_period = 0;

    inline void update_sweep(unsigned char val){
        sweep_enable = val & 0x80;
        sweep_period = (val >> 4) & 0x7;
        sweep_is_neg = val & 0x8;
        sweep_shift = val & 0x7;
        sweep_reset = true;
    }

    inline void sweep_update_timer(){
        sweep_reset = false;
        sweep_counter = sweep_period;
        sweep_delta = timer >> sweep_shift;
        // TODO: check + pulse1/pulse2 update
        int res = sweep_is_neg ? std::max(timer - sweep_delta, 0) : timer + sweep_delta;
        sweep_target_period = (unsigned short)res;
        if (timer >= 8 && sweep_target_period <= 0x7ff && sweep_enable && sweep_shift)
            timer = sweep_target_period;
    }

    unsigned char current_duty_step{};
    inline void tick_timer(Bus& bus){
        --timer;
        if (!timer){
            current_duty_step = !current_duty_step ? 7 : current_duty_step - 1;
            update_timer(bus.unsafe_read_prg(timer_l), bus.unsafe_read_prg(timer_h));
        }
    }

    inline void update_lc_and_timer(Bus& bus){
        length_counter = Bus::length_table[((bus.unsafe_read_prg(timer_h) >> 3) & 0x1f)];
        update_timer(bus.unsafe_read_prg(timer_l), bus.unsafe_read_prg(timer_h));
    }

    inline unsigned char get_duty_cycle(Bus& bus) const {
        switch((bus.unsafe_read_prg(duty_envelope) >> 6) & 0x3){
            case 0:  return  0b0000'0001; // 12.5%
            case 1:  return  0b0000'0011; // 25%
            case 2:  return  0b0000'1111; // 50%
            default: return  0b1111'1100; // 25% (negated)
        }
    }
    inline unsigned char get_duty_value(Bus& bus) const {
        return (get_duty_cycle(bus) >> current_duty_step) & 1;
    }
    inline unsigned char get_dac(Bus& bus) const {
        // TODO: check
        if (length_counter && timer >= 8 && sweep_target_period <= 0x7ff && get_duty_value(bus))
            return  get_volume(bus) & 0xf;
        return 0;
    }
};

//https://www.nesdev.org/wiki/APU_Noise
struct NoiseChan : Channel, Envelope {
    unsigned char loop{};
    bool mode = false;
    static constexpr unsigned short period[16]{
        4,8,16,32,64,96,128,160,202,254,380,508,762,
        1016,2034,4068
    };
    unsigned short shift_reg = 1;
    inline void tick_timer(Bus& bus){
        --timer;
        if (!timer){
            bool feedback = (shift_reg & 1) ^ (mode ? (!!(shift_reg & 0x40)) : (!!(shift_reg & 0x2)));
            shift_reg >>= 1;
            shift_reg |= (feedback << 14);
            timer = period[(loop & 0xf)];
        }
    }
    inline void update_lc_and_timer(Bus& bus){
        length_counter = Bus::length_table[((bus.unsafe_read_prg(constants::NOISE_CHAN_LENGTH_COUNTER) >> 3) & 0x1f)];
        timer = period[(loop & 0xf)]; // TODO: check
    }
    inline unsigned char get_dac(Bus& bus) const {
        // TODO: check
        if (length_counter && !(shift_reg & 1))
            return get_volume(bus) & 0xf;
        return 0;
    }
};

struct TriangleChan : Channel{
    unsigned char next_linear_counter{};
    unsigned char linear_counter{};
    bool linear_counter_reload = false;
    static constexpr unsigned char seq[32]{
        15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
    };
    size_t current_seq_i{};
    inline void tick_timer(Bus& bus){
        --timer;
        if (!timer){
            if (linear_counter) current_seq_i = (current_seq_i + 1) & 0x1f;
            update_timer(bus.unsafe_read_prg(constants::TRIANGLE_CHAN_TIMER_LOW), bus.unsafe_read_prg(constants::TRIANGLE_CHAN_TIMER_HIGH));
        }
    }
    inline void update_lc_and_timer(Bus& bus){
        length_counter = Bus::length_table[((bus.unsafe_read_prg(constants::TRIANGLE_CHAN_TIMER_HIGH) >> 3) & 0x1f)];
        update_timer(bus.unsafe_read_prg(constants::TRIANGLE_CHAN_TIMER_LOW), bus.unsafe_read_prg(constants::TRIANGLE_CHAN_TIMER_HIGH));
    }
    unsigned char get_dac(){
        // TODO: check
        if (length_counter)
            return seq[current_seq_i];
        return 0;
    }
};

// TODO: finish
struct DMCChan : Channel {
    bool is_loop{};
    bool irq_enable{};
    unsigned short dma_base_address{};
    void tick(Bus& bus);
};

struct APU {
    short buffer[1024]{};
    size_t write_i = 0;
    size_t read_i = 0;
    size_t sample_count = 0;
    size_t sample_acc = 0;

#if DEBUG_MODE
    unsigned char internal_enable = 0x1f;
#endif

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
    size_t irq_set_counter = 0;

    void tick(Bus& bus);
    void handle_frame_counter_update(Bus& bus);
    void frame_counter_step4(Bus& bus);
    void frame_counter_step5(Bus& bus);
    void frame_counter_quarter_update(Bus& bus);
    void frame_counter_half_update(Bus& bus);
    float gen_sample(Bus& bus);
    void update_control(Bus& bus);
};
#endif
