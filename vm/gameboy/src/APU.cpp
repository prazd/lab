#include "APU.h"

void SquareWaveChannel::update_duty(const Memory& mem, unsigned short address) {
    switch ((mem.read_unsafe(address) >> 6) & 0x3) {
        case 0b00: {duty = 0b0000'0001;  break;} // 12.5%
        case 0b01: {duty = 0b1000'0001;  break;} // 25%
        case 0b10: {duty = 0b1000'0111;  break;} // 50%
        case 0b11: {duty = 0b0111'1110;  break;} // 75%
    }
}

void SquareWaveChannel::update_pitch(const Memory& mem, unsigned short l_addr, unsigned short h_addr) {
    unsigned char l = mem.read_unsafe(l_addr);
    unsigned char h = (mem.read_unsafe(h_addr) & 0x7);
    auto x = ((h << 8) | l);

    shadow_frequency = x;
    freq_timer = (2048 - x) * 4;
    freq_timer_period = freq_timer;
}

void CH1::init(Memory& mem) {
    update_duty(mem, constants::NR11_ADDRESS);
    update_pitch(mem, constants::NR13_ADDRESS, constants::NR14_ADDRESS);
    phase = 0;
    length_counter = 64 - (mem.read_unsafe(constants::NR11_ADDRESS) & 0x3f);
    freq_timer = freq_timer_period;
    envelope_volume = (mem.read_unsafe(constants::NR12_ADDRESS) >> 4) & 0xf;
    envelope_timer = (mem.read_unsafe(constants::NR12_ADDRESS) & 0x7);
    mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) | 1);
}

void CH2::init(const Memory& mem) {
    update_duty(mem, constants::NR21_ADDRESS);
    update_pitch(mem, constants::NR23_ADDRESS, constants::NR24_ADDRESS);
    phase = 0;
    length_counter = 64 - (mem.read_unsafe(constants::NR21_ADDRESS) & 0x3f);
    freq_timer = freq_timer_period;
    envelope_volume = (mem.read_unsafe(constants::NR22_ADDRESS) >> 4) & 0xf;
    envelope_timer = (mem.read_unsafe(constants::NR22_ADDRESS) & 0x7);
}

void CH1::disable(Memory& mem) {
    mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) & 0xE);
    envelope_volume = 0;
    active = false;
    dac_out = 0.f;
}

void CH2::disable(Memory& mem) {
    mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) & 0xD);
    envelope_volume = 0;
    active = false;
    dac_out = 0.f;
}

void CH1::disable_check(Memory& mem) {
    if (length_counter == 0 && (mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1) disable(mem);
}

void CH2::disable_check(Memory& mem) {
    if (length_counter == 0 && (mem.read_unsafe(constants::NR24_ADDRESS) >> 6) & 1) disable(mem);
}

void CH1::sweep_step(Memory &mem, bool init) {
    auto nr10 = mem.read_unsafe(constants::NR10_ADDRESS);
    sweep_timer = ((nr10 >> 4) & 0x7);
    unsigned char shift = nr10 & 0x7;
    if (shift == 0 && sweep_timer == 0) return;
    if (sweep_timer == 0) sweep_timer = 9; // todo: debug (should be 8)
    if (shift == 0 && init) {
        unsigned char l = mem.read_unsafe(constants::NR13_ADDRESS);
        unsigned char h = (mem.read_unsafe(constants::NR14_ADDRESS) & 0x7);
        shadow_frequency =  ((h << 8) | l);
        return;
    }
    auto delta = shadow_frequency >> shift;
    unsigned new_val{};
    if (!((nr10 >> 3) & 1)) new_val = shadow_frequency + delta;
    else new_val = shadow_frequency - delta;
    if (new_val > 2047){
        disable(mem);
    }
    shadow_frequency = new_val;
    mem.write_unsafe(constants::NR13_ADDRESS, new_val & 0xff);
    mem.write_unsafe(constants::NR14_ADDRESS,
                     (mem.read_unsafe(constants::NR14_ADDRESS) & 0xf8) | ((new_val >> 8) & 0x7));
    // todo: maybe second overflow check
    update_pitch(mem, constants::NR13_ADDRESS, constants::NR14_ADDRESS);
}

void CH3::update_length_timer(const Memory &mem) {
    length_counter = (mem.read_unsafe(constants::NR31_ADDRESS));
}

void CH4::update_length_timer(const Memory &mem) {
    length_counter = (mem.read_unsafe(constants::NR41_ADDRESS) & 0x3f);
}

void APU::frame_seq_tick(Memory& mem) {
    if (frame_seq_t_cycle++ % 8192 == 0)
        frame_seq_handler(mem, std::exchange(frame_sequencer_step, (frame_sequencer_step + 1) % 8));
}

void APU::frame_seq_handler(Memory& mem, unsigned char step) {
    switch (step) {
        case 0: {
            if (ch1.active && (mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1) --ch1.length_counter;
            if (ch2.active && (mem.read_unsafe(constants::NR24_ADDRESS) >> 6) & 1) --ch2.length_counter;
            break;
        }
        case 2: {
            if (ch1.active) {
                if ((mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1) --ch1.length_counter;
                if (!(--ch1.sweep_timer)) ch1.sweep_step(mem, false);
            }
            if (ch2.active && (mem.read_unsafe(constants::NR24_ADDRESS) >> 6) & 1) --ch2.length_counter;
            break;
        }
        case 4: {
            if (ch1.active && (mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1)  --ch1.length_counter;
            if (ch2.active && (mem.read_unsafe(constants::NR24_ADDRESS) >> 6) & 1)  --ch2.length_counter;
            break;
        }
        case 6: {
            if (ch1.active){
                if ((mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1) --ch1.length_counter;
                if (!(--ch1.sweep_timer)) ch1.sweep_step(mem, false);
            }
            if (ch2.active && (mem.read_unsafe(constants::NR24_ADDRESS) >> 6) & 1)  --ch2.length_counter;
            break;
        }
        case 7: {
            if (!ch1.envelope_timer) {
                if (!(mem.read_unsafe(constants::NR12_ADDRESS) & 0x7))
                    ch1.envelope_volume = 0;
                else {
                    ch1.envelope_volume = std::clamp((mem.read_unsafe(constants::NR12_ADDRESS) >> 3) & 1 ?
                                                     ch1.envelope_volume + 1 : ch1.envelope_volume - 1, 0, 15);
                }
                ch1.dac_out = (ch1.duty >> ch1.phase) & 1 ? ch1.envelope_volume : 0;
                ch1.envelope_timer = (mem.read_unsafe(constants::NR12_ADDRESS) & 0x7);
            } else {
                --ch1.envelope_timer;
            }

            if (!ch2.envelope_timer) {
                if (!(mem.read_unsafe(constants::NR22_ADDRESS) & 0x7))
                    ch1.envelope_volume = 0;
                else {
                    ch2.envelope_volume = std::clamp((mem.read_unsafe(constants::NR22_ADDRESS) >> 3) & 1 ?
                                                     ch2.envelope_volume + 1 : ch2.envelope_volume - 1, 0, 15);
                }
                ch2.dac_out = (ch2.duty >> ch2.phase) & 1 ? ch2.envelope_volume : 0;
                ch2.envelope_timer = (mem.read_unsafe(constants::NR22_ADDRESS) & 0x7);
            } else {
                --ch2.envelope_timer;
            }
            break;
        }
        default: break;
    }
    ch1.disable_check(mem);
    ch2.disable_check(mem);
}

static float next_square_sample(float dac_value){
    return (dac_value/15.f);
}

void APU::tick(Memory& mem) {
    static unsigned t_cycle = 0;
    static int current = 0;
    static unsigned char prev_frame_seq_step = 0;
    static bool skip_sample = false;
    auto master_control =  mem.read_unsafe(constants::NR52_ADDRESS);

//    if (!((master_control >> 7) & 1)) {
//        ch1.disable();
//        ch2.disable();
//    }

    if (mem.ch1_trigger
         && (mem.read_unsafe(constants::NR12_ADDRESS) & 0xf8) != 0) {
        ch1.init(mem);
        ch1.active = true;
        mem.ch1_trigger = false;

        // sweep overflow check
        ch1.sweep_step(mem, true);
    }

    if (mem.ch2_trigger
        && (mem.read_unsafe(constants::NR22_ADDRESS) & 0xf8) != 0) {
        mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) | 1 << 1);
        ch2.init(mem);
        ch2.active = true;
        mem.ch2_trigger = false;
    }

    if (ch1.active) {
        if (!(--ch1.freq_timer)) {
            ch1.phase = (ch1.phase + 1) % 8;
            ch1.freq_timer = ch1.freq_timer_period;
            ch1.dac_out = (ch1.duty >> ch1.phase) & 1 ? ch1.envelope_volume : 0;
        }
    }

    if (ch2.active) {
        if (!(--ch2.freq_timer)) {
            ch2.phase = (ch2.phase + 1) % 8;
            ch2.freq_timer = ch2.freq_timer_period;
            ch2.dac_out = (ch2.duty >> ch2.phase) & 1 ? ch2.envelope_volume : 0;
        }
    }

    if (t_cycle++ % 95 == 0) {
            auto mix = 0.f;
            float count = 0.f;
            if (ch1.active){
#if DEBUG_MODE
                if (mem.custom_ch1_active){
                    mix += (next_square_sample(ch1.dac_out));
                    count+=1.f;
                }
#else
                mix += (next_square_sample(ch1.dac_out));
                count+=1.f;
#endif
            }
            if (ch2.active){
#if DEBUG_MODE
                if (mem.custom_ch2_active){
                    mix += (next_square_sample(ch2.dac_out));
                    count+=1.f;
                }
#else
                mix += (next_square_sample(ch2.dac_out));
                count+=1.f;
#endif
            }

            mix /= count == 0 ? 1.f : count;
            if (mix > 1.f) {
                mix = 1.f;
            } else if (mix < -1.f) {
                mix = -1.f;
            }
            buffer[current = (current + 1) % 2048] = (short)(mix * 32767.f);
    }
}