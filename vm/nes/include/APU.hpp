#ifndef NES_APU_HPP
#define NES_APU_HPP
#include "Bus.hpp"

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
    static constexpr unsigned short ntsc_rates[] = {
        428,380,340,320,286,254,226,214,190,160,142,128,106,84,72,54
    };
    bool is_loop{};
    bool irq_enable{};
    unsigned char rate_index{};

    unsigned short rate_counter{};
    unsigned short dma_address{};
    unsigned short bytes_remaining{};

    bool need_sample_update = false;
    bool sample_ready{};
    unsigned char sample_buffer{};
    unsigned char shift_reg{};
    unsigned char shift_count{};
    char output_level{};

    unsigned char dmc_dma_counter = 0;

    inline void restart(Bus& bus){
        if (bytes_remaining) return;
        auto s_len = bus.unsafe_read_prg(constants::DMC_CHAN_SAMPLE_LENGTH);
        bytes_remaining = (s_len << 4) | 1;
        auto chan_direct_load_val = bus.unsafe_read_prg(constants::DMC_CHAN_DIRECT_LOAD);
        output_level = (char)(chan_direct_load_val & 0x7f);
        rate_counter = ntsc_rates[rate_index];
        dma_address = 0xc000;
        auto s_addr = bus.unsafe_read_prg(constants::DMC_CHAN_SAMPLE_ADDRESS);
        dma_address |= s_addr << 6;
        dmc_dma_counter = 4;
        sample_ready = false;
        bus.dmc_dma_in_progress = true;
    }

    inline void tick(Bus& bus){
        auto chan_freq_val = bus.unsafe_read_prg(constants::DMC_CHAN_FREQ);
        is_loop = chan_freq_val & 0x40;
        irq_enable = chan_freq_val & 0x80;
        if (!irq_enable) bus.apu_control &= 0x7f;
        rate_index = chan_freq_val & 0xf;

        if (!bytes_remaining && is_loop){
            auto s_len = bus.unsafe_read_prg(constants::DMC_CHAN_SAMPLE_LENGTH);
            bytes_remaining = (s_len << 4) | 1;
            dma_address = 0xc000;
            auto s_addr = bus.unsafe_read_prg(constants::DMC_CHAN_SAMPLE_ADDRESS);
            dma_address |= s_addr << 6;
            if (!sample_ready){
                bus.apu_irq_interrupt_flip_flop = true;
                sample_ready = false;
                dmc_dma_counter = 4;
                need_sample_update = true;
            }
        } else if (!bytes_remaining){
            return;
        }

        if (dmc_dma_counter) {
            --dmc_dma_counter;
            bus.dmc_dma_read_prg(bus.dma_address); // dummy read
            if (!dmc_dma_counter){
                sample_ready = true;
                sample_buffer = bus.dmc_dma_read_prg(dma_address);
                bus.cpu_open_bus_value = sample_buffer;
                dma_address = dma_address == 0xffff ? 0x8000 : dma_address + 1;
                if (bytes_remaining)
                    --bytes_remaining;
                bus.dmc_dma_in_progress = false;
                if (bytes_remaining){
                    if (need_sample_update){
                        shift_reg = sample_buffer;
                        shift_count = 8;
                        // todo: check put/get cycle
                        bus.dmc_dma_in_progress = true;
                        sample_ready = false;
                        dmc_dma_counter = 4;
                        need_sample_update = false;
                    }
                } else if (!bytes_remaining && !is_loop && irq_enable){
                    bus.apu_control &= 0x7f;
                    bus.apu_control |= 0x80;
                    bus.apu_irq_interrupt_flip_flop = true;
                }
            }
        }

        if (rate_counter) --rate_counter;
        if (!rate_counter){
            output_level += (shift_reg & 1) ? 2 : -2;
            if (shift_count){
                shift_reg >>= 1;
                --shift_count;
                output_level = std::clamp(output_level, (char)127, (char)0);
                rate_counter = ntsc_rates[rate_index];
            }
            if (!shift_count){
                if (!sample_ready)
                    need_sample_update = true;
                else {
                    shift_reg = sample_buffer;
                    shift_count = 8;
                }
                // todo: check put/get cycle
//                if (!bus.dmc_dma_in_progress){
                    bus.dmc_dma_in_progress = true;
                    sample_ready = false;
                    dmc_dma_counter = 4;
//                }
            }
        }
    }
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
    bool frame_counter_reset = false;
    unsigned char frame_counter_reset_ticks = 0;

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
    void update_control(Bus& bus);

    static constexpr unsigned char length_table[32]{
        10,
        254,
        20,
        2,
        40,
        4,
        80,
        6,
        160,
        8,
        60,
        10,
        14,
        12,
        26,
        14,
        12,
        16,
        24,
        18,
        48,
        20,
        96,
        22,
        192,
        24,
        72,
        26,
        16,
        28,
        32,
        30
    };
};
#endif
