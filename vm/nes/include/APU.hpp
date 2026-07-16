#ifndef NES_APU_HPP
#define NES_APU_HPP
#include "Bus.hpp"
#include "Simple.hpp"

struct Envelope {
    unsigned char prev_divider{};
    unsigned char divider{};
    unsigned char decay_lvl = 15;
    unsigned char envelope{};
    bool is_volume_const = false;
    unsigned char volume_const_val = 0;
    bool is_infinite_play = false;
    inline void update_envelope(Bus& bus, unsigned char val){
        is_volume_const = val & 0x10;
        volume_const_val = val & 0xf;
        is_infinite_play = val & 0x20;
        prev_divider = val & 0xf;
    }
    inline unsigned char get_volume(Bus& bus){
        return is_volume_const ? volume_const_val : decay_lvl;
    }
    inline void envelope_tick(Bus& bus){
        if (bus.apu_pulse1_start_flag){
            divider = prev_divider;
            decay_lvl = 15;
            bus.apu_pulse1_start_flag = false;
            return;
        }
        if (divider) --divider;
        if (!divider){
            divider = prev_divider;
            if (decay_lvl) --decay_lvl;
            if (!decay_lvl) {
                if (is_infinite_play) decay_lvl = 15;
            }
        }
    }
};

struct Channel {
    unsigned short timer{};
    unsigned char length_counter{};
    unsigned char sweep_shift{};
    inline void update_timer(unsigned char l, unsigned short h){
        timer = (((h & 0x7) << 8) | l);
    }
};

template <unsigned short timer_l,
          unsigned short timer_h,
          unsigned short duty_envelope>
struct PulseChan : Channel, Envelope {
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

// TODO: finish
struct DMCChan : Channel {
    bool is_loop{};
    bool irq_enable{};
    bool need_sample_update = false;

    // TODO: debug move to bus

    inline void tick(Bus& bus){
        auto chan_freq_val = bus.unsafe_read_prg(constants::DMC_CHAN_FREQ);
        is_loop = chan_freq_val & 0x40;
        irq_enable = chan_freq_val & 0x80;
        if (!irq_enable) bus.apu_control &= 0x7f;
        bus.dmc_rate_index = chan_freq_val & 0xf;

        if (!bus.dmc_bytes_remaining && is_loop){
            auto s_len = bus.unsafe_read_prg(constants::DMC_CHAN_SAMPLE_LENGTH);
            bus.dmc_bytes_remaining = (s_len << 4) | 1;
            bus.dmc_dma_address = 0xc000;
            auto s_addr = bus.unsafe_read_prg(constants::DMC_CHAN_SAMPLE_ADDRESS);
            bus.dmc_dma_address |= s_addr << 6;
            if (bus.dmc_sample_buffer.empty()){
                bus.apu_irq_interrupt_flip_flop = true;
                bus.dmc_dma_counter = 3 + bus.next_write;
                need_sample_update = true;
            }
        } else if (!bus.dmc_bytes_remaining){
            return;
        }

        if (bus.dmc_delay){
            if (!(bus.dmc_delay == 1 && bus.next_write)) --bus.dmc_delay;
            if (!bus.dmc_delay) {bus.dmc_dma_counter = 3 + bus.next_write; bus.dmc_dma_in_progress = true;}
        }

        if (bus.dmc_dma_counter && bus.dmc_dma_in_progress) {
            --bus.dmc_dma_counter;
            if (bus.last_arg_addr == 0x4016){
                // TODO: debug
                if (bus.dmc_dma_counter == 3 && !(bus.apu_current_tick & 1)){
                    bus.dmc_dma_read_prg(bus.last_arg_addr); // dummy read
                }
            } else {
                bus.dmc_dma_read_prg(bus.last_arg_addr); // dummy read
            }
            if (!bus.dmc_dma_counter){
                bus.dmc_sample_buffer.push_back(bus.dmc_dma_read_prg(bus.dmc_dma_address));
                bus.dmc_dma_address = bus.dmc_dma_address == 0xffff ? 0x8000 : bus.dmc_dma_address + 1;
                if (bus.dmc_bytes_remaining)
                    --bus.dmc_bytes_remaining;
                bus.dmc_dma_in_progress = false;
                if (bus.dmc_bytes_remaining){
                    if (need_sample_update){
                        bus.dmc_shift_reg = bus.dmc_sample_buffer.back(); bus.dmc_sample_buffer.pop_back();
                        bus.dmc_shift_count = 8;
                    }
                } else if (!bus.dmc_bytes_remaining && !is_loop && irq_enable){
                    bus.apu_control &= 0x7f;
                    bus.apu_control |= 0x80;
                    bus.apu_irq_interrupt_flip_flop = true;
                }
            } else {
                return;
            }
        }

        if (!bus.dmc_rate_counter){
            bus.dmc_output_level += (bus.dmc_shift_reg & 1) ? 2 : -2;
            if (bus.dmc_shift_count){
                bus.dmc_shift_reg >>= 1;
                --bus.dmc_shift_count;
                bus.dmc_output_level = std::clamp(bus.dmc_output_level, (char)127, (char)0);
                bus.dmc_rate_counter = Bus::ntsc_rates[bus.dmc_rate_index];
            }
            if (!bus.dmc_shift_count){
                if (bus.dmc_sample_buffer.empty()){
                    need_sample_update = true;
                    bus.dmc_dma_counter = 3 + bus.next_write;
                    bus.dmc_dma_in_progress = true;
                } else {
                    bus.dmc_shift_reg = bus.dmc_sample_buffer.back(); bus.dmc_sample_buffer.pop_back();
                    bus.dmc_shift_count = 8;
                }
                // todo: check put/get cycle
            }
        }
    }
};

struct APU {
    short buffer[1024]{};
    size_t buffer_i = 0;
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
