#include "APU.h"
#include <cmath>
#include <numeric>

void SquareWaveChannel::update_duty(const Memory& mem, unsigned short address) {
    // https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware
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
    if (!length_counter)
        length_counter = 64;
    freq_timer = freq_timer_period;
    envelope_volume = (mem.read_unsafe(constants::NR12_ADDRESS) >> 4) & 0xf;
    envelope_timer = (mem.read_unsafe(constants::NR12_ADDRESS) & 0x7);
    if (!envelope_timer) envelope_timer = 8;
}


void CH2::init(const Memory& mem) {
    update_duty(mem, constants::NR21_ADDRESS);
    update_pitch(mem, constants::NR23_ADDRESS, constants::NR24_ADDRESS);
    phase = 0;
    if (!length_counter)
        length_counter = 64;
    freq_timer = freq_timer_period;
    envelope_volume = (mem.read_unsafe(constants::NR22_ADDRESS) >> 4) & 0xf;
    envelope_timer = (mem.read_unsafe(constants::NR22_ADDRESS) & 0x7);
    if (!envelope_timer) envelope_timer = 8;
}


void CH3::init(Memory& mem) {
    nibble = 0;
    wave = 0;
    freq = ((mem.read_unsafe(constants::NR34_ADDRESS) & 0x7) << 8) | (mem.read_unsafe(constants::NR33_ADDRESS));
    if (!length_counter) length_counter = 256;
    freq_timer = (2048 - freq) * 2;
}

void CH4::init(const Memory& mem) {
    if (!length_counter)
        length_counter = 64;
    envelope_volume = (mem.read_unsafe(constants::NR42_ADDRESS) >> 4) & 0xf;
    envelope_timer = (mem.read_unsafe(constants::NR42_ADDRESS) & 0x7);
    if (!envelope_timer) envelope_timer = 8;
    auto divider = (mem.read_unsafe(constants::NR43_ADDRESS) & 0x7);
    unsigned char shift = (mem.read_unsafe(constants::NR43_ADDRESS) >> 4) & 0xf;
    freq_period = (dividers[divider] << shift);
    freq_timer = freq_period;
    lsfr_width = ((mem.read_unsafe(constants::NR43_ADDRESS) >> 3) & 1);
    lsfr = 0x7FFF;
}

void CH1::disable(Memory& mem) {
    mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) & 0xFE);
    envelope_volume = 0;
    active = false;
    dac_out = 0.f;
}

void CH2::disable(Memory& mem) {
    mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) & 0xFD);
    envelope_volume = 0;
    active = false;
    dac_out = 0.f;
}

void CH3::disable(Memory &mem) {
    mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) & 0xFB);
    active = false;
    dac_out = 0.f;
}

void CH4::disable(Memory &mem) {
    mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) & 0xF7);
    envelope_volume = 0;
    active = false;
    freq_timer = 0;
}

void CH1::disable_check(Memory& mem) {
    if (length_counter == 0 && active) disable(mem);
}

void CH2::disable_check(Memory& mem) {
//    if ( ((length_counter == 0 && (mem.read_unsafe(constants::NR24_ADDRESS) >> 6) & 1)
//    || !((mem.read_unsafe(constants::NR22_ADDRESS) >> 4) & 0xf) && active) && active)  disable(mem);
    if (length_counter == 0 && active) disable(mem);
}

void CH3::disable_check(Memory &mem) {
    if (length_counter == 0 && active) disable(mem);
}

void CH4::disable_check(Memory &mem) {
    if (length_counter == 0 && active) disable(mem);
}

void CH1::sweep_step(Memory &mem, bool init) {
    auto nr10 = mem.read_unsafe(constants::NR10_ADDRESS);
    sweep_timer = ((nr10 >> 4) & 0x7);
    unsigned char shift = nr10 & 0x7;
    if (shift == 0 && sweep_timer == 0) return;
    if (sweep_timer == 0) sweep_timer = 8; // todo: debug (should be 8)
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
    update_pitch(mem, constants::NR13_ADDRESS, constants::NR14_ADDRESS);

    // Second check
    if (shift != 0) {
        delta = shadow_frequency >> shift;
        if (!((nr10 >> 3) & 1)) new_val = shadow_frequency + delta;
        else new_val = shadow_frequency - delta;
        if (new_val > 2047){
            disable(mem);
        }
    }
}

void APU::frame_seq_tick(Memory& mem) {
    if (frame_seq_t_cycle++ % 8192 == 0)
        frame_seq_handler(mem, std::exchange(frame_sequencer_step, (frame_sequencer_step + 1) % 8));
}

void APU::frame_seq_handler(Memory& mem, unsigned char step) {
    switch (step) {
        case 0: {
            if ((mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1)  --ch1.length_counter;
            if ((mem.read_unsafe(constants::NR24_ADDRESS) >> 6) & 1)  --ch2.length_counter;
            if ((mem.read_unsafe(constants::NR34_ADDRESS) >> 6) & 1)  --ch3.length_counter;
            if ((mem.read_unsafe(constants::NR44_ADDRESS) >> 6) & 1)  --ch4.length_counter;
            break;
        }
        case 2: {
            if ((mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1)   --ch1.length_counter;
            if (!(--ch1.sweep_timer)) ch1.sweep_step(mem, false);
            if ((mem.read_unsafe(constants::NR24_ADDRESS) >> 6) & 1)   --ch2.length_counter;
            if ((mem.read_unsafe(constants::NR34_ADDRESS) >> 6) & 1)   --ch3.length_counter;
            if ((mem.read_unsafe(constants::NR44_ADDRESS) >> 6) & 1)   --ch4.length_counter;
            break;
        }
        case 4: {
            if ((mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1)   --ch1.length_counter;
            if ((mem.read_unsafe(constants::NR24_ADDRESS) >> 6) & 1)   --ch2.length_counter;
            if ((mem.read_unsafe(constants::NR34_ADDRESS) >> 6) & 1)   --ch3.length_counter;
            if ((mem.read_unsafe(constants::NR44_ADDRESS) >> 6) & 1)   --ch4.length_counter;
            break;
        }
        case 6: {
            if ((mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1)   --ch1.length_counter;
            if (!(--ch1.sweep_timer)) ch1.sweep_step(mem, false);
            if ((mem.read_unsafe(constants::NR24_ADDRESS) >> 6) & 1)   --ch2.length_counter;
            if ((mem.read_unsafe(constants::NR34_ADDRESS) >> 6) & 1)   --ch3.length_counter;
            if ((mem.read_unsafe(constants::NR44_ADDRESS) >> 6) & 1)   --ch4.length_counter;
            break;
        }
        case 7: {
            --ch1.envelope_timer;
            if (!ch1.envelope_timer) {
                if ((mem.read_unsafe(constants::NR12_ADDRESS) & 0x7))
                    ch1.envelope_volume = std::clamp((mem.read_unsafe(constants::NR12_ADDRESS) >> 3) & 1 ?
                                                     (char)(ch1.envelope_volume + 1) : (char)(ch1.envelope_volume - 1), (char)0, (char)15);
                ch1.envelope_timer = (mem.read_unsafe(constants::NR12_ADDRESS) & 0x7);
                if (!ch1.envelope_timer) ch1.envelope_timer = 8;
            }

            --ch2.envelope_timer;
            if (!ch2.envelope_timer) {
                if ((mem.read_unsafe(constants::NR22_ADDRESS) & 0x7)){
                    ch2.envelope_volume = std::clamp((mem.read_unsafe(constants::NR22_ADDRESS) >> 3) & 1 ?
                                                     (char)(ch2.envelope_volume + 1) : (char)(ch2.envelope_volume - 1), (char)0, (char)15);
                }
                ch2.envelope_timer = (mem.read_unsafe(constants::NR22_ADDRESS) & 0x7);
                if (!ch2.envelope_timer) ch2.envelope_timer = 8;
            }

            --ch4.envelope_timer;
            if (!ch4.envelope_timer) {
                if ((mem.read_unsafe(constants::NR42_ADDRESS) & 0x7)){
                    ch4.envelope_volume = std::clamp((mem.read_unsafe(constants::NR42_ADDRESS) >> 3) & 1 ?
                                                     (char)(ch4.envelope_volume + 1) : (char)(ch4.envelope_volume - 1), (char)0, (char)15);
                }
                ch4.envelope_timer = (mem.read_unsafe(constants::NR42_ADDRESS) & 0x7);
                if (!ch4.envelope_timer) ch4.envelope_timer = 8;
            }
            break;
        }
        default: break;
    }
    ch1.disable_check(mem);
    ch2.disable_check(mem);
    ch3.disable_check(mem);
    ch4.disable_check(mem);
}


template <bool left>
float highpass(float x) {
    static float in_prev = 0.f;
    static float y_prev = 0.f;
    static float x_prev = 0.f;
    static constexpr float a = 0.996f;
    float res = x - x_prev + (a * y_prev);
    y_prev = res;
    x_prev = x;
    return res;
}

std::pair<float, float> APU::next_sample_2ch(Memory &mem) {
    auto nr51 = mem.read_unsafe(constants::NR51_ADDRESS);
    auto mix_left = 0.f;
    auto mix_right = 0.f;
    bool ch1_right_active = (nr51) & 1;
    bool ch2_right_active = ((nr51)>>1) & 1;
    bool ch3_right_active = ((nr51)>>2) & 1;
    bool ch4_right_active = ((nr51)>>3) & 1;

    bool ch1_left_active = ((nr51)>>4) & 1;
    bool ch2_left_active = ((nr51)>>5) & 1;
    bool ch3_left_active = ((nr51)>>6) & 1;
    bool ch4_left_active = ((nr51)>>7) & 1;

    if (ch1.active){
#if DEBUG_MODE
        if (mem.custom_ch1_active){
            mix_left += ch1_left_active ? ch1.dac_out/15.f : 0;
            mix_right += ch1_right_active ? ch1.dac_out/15.f : 0;
        }
#else
        mix_left += ch1_left_active ? ch1.dac_out/15.f : 0;
        mix_right += ch1_right_active ? ch1.dac_out/15.f : 0;
#endif
    }
    if (ch2.active){
#if DEBUG_MODE
        if (mem.custom_ch2_active){
            mix_left += ch2_left_active ? ch2.dac_out/15.f : 0;
            mix_right += ch2_right_active ?  ch2.dac_out/15.f : 0;
        }
#else
        mix_left += ch2_left_active ? ch2.dac_out/15.f : 0;
        mix_right += ch2_right_active ? ch2.dac_out/15.f : 0;
#endif
    }

    if (ch3.active){
#if DEBUG_MODE
        if (mem.custom_ch3_active){
            mix_left += ch3_left_active ? (ch3.dac_out/15.f)/2 : 0;
            mix_right += ch3_right_active ? (ch3.dac_out/15.f)/2: 0;
        }
#else
        mix_left += ch3_left_active ?   (ch3.dac_out/15.f)/2 : 0;
        mix_right += ch3_right_active ?  (ch3.dac_out/15.f)/2 : 0;
#endif
    }


    if (ch4.active){
#if DEBUG_MODE
        if (mem.custom_ch4_active){
            mix_left += ch4_left_active ? ch4.dac_out/15.f/2 : 0;
            mix_right += ch4_right_active ? ch4.dac_out/15.f/2 : 0;
        }
#else
        mix_left += ch4_left_active ?  ch4.dac_out/15.f/2 : 0;
        mix_right += ch4_right_active ? ch4.dac_out/15.f/2 : 0;
#endif
    }
    mix_left  = std::clamp(mix_left , -1.f, 1.f);
    mix_right = std::clamp(mix_right, -1.f, 1.f);
    return {mix_left, mix_right};
}

void APU::tick(Memory& mem) {
    static unsigned t_cycle = 0;
    static unsigned char prev_frame_seq_step = 0;
    static bool skip_sample = false;

    if (!(mem.read_unsafe(constants::NR12_ADDRESS) & 0xf8) && ch1.active) ch1.disable(mem);
    if (!(mem.read_unsafe(constants::NR22_ADDRESS) & 0xf8) && ch2.active) ch2.disable(mem);
    if (!((mem.read_unsafe(constants::NR30_ADDRESS) >> 7) & 1) && ch3.active) ch3.disable(mem);
    if (!(mem.read_unsafe(constants::NR42_ADDRESS) & 0xf8) && ch4.active) ch4.disable(mem);


    if (mem.ch1_trigger
         && (mem.read_unsafe(constants::NR12_ADDRESS) & 0xf8) != 0 && mem.read_unsafe(constants::NR52_ADDRESS)) {
        mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) | 1);
        ch1.init(mem);
        ch1.active = true;
        mem.ch1_trigger = false;
        // sweep overflow check
        ch1.sweep_step(mem, true);
    }

    if (mem.ch2_trigger
        && (mem.read_unsafe(constants::NR22_ADDRESS) & 0xf8) != 0) {
        mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) | (1 << 1));
        ch2.init(mem);
        ch2.active = true;
        mem.ch2_trigger = false;
    }

    if (mem.ch3_trigger) {
        ch3.init(mem);
        mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) | (1 << 2));
        ch3.active = true;
        mem.ch3_trigger = false;
    }

    if (mem.ch4_trigger) {
        ch4.init(mem);
        mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) | (1 << 3));
        ch4.active = true;
        mem.ch4_trigger = false;
    }


    if (mem.ch1_update_timer) {
        ch1.length_counter = 64 - (mem.read_unsafe(constants::NR11_ADDRESS) & 0x3f);
        mem.ch1_update_timer = false;
    }

    if (mem.ch2_update_timer) {
        ch2.length_counter = 64 - (mem.read_unsafe(constants::NR21_ADDRESS) & 0x3f);
        mem.ch2_update_timer = false;
    }

    if (mem.ch3_update_timer) {
        ch3.length_counter = 256 - mem.read_unsafe(constants::NR31_ADDRESS);
        mem.ch3_update_timer = false;
    }

    if (mem.ch4_update_timer) {
        ch4.length_counter = 64 - (mem.read_unsafe(constants::NR41_ADDRESS) & 0x3f);
        mem.ch4_update_timer = false;
    }

    if (ch1.active) {
        if (!(--ch1.freq_timer)) {
            ch1.phase = (ch1.phase + 1) % 8;
            ch1.freq_timer = ch1.freq_timer_period;
            ch1.dac_out = (ch1.duty >> (ch1.phase)) & 1 ? ch1.envelope_volume : 0;
        }
    }

    if (ch2.active) {
        if (!(--ch2.freq_timer)) {
            ch2.phase = (ch2.phase + 1) % 8;
            ch2.freq_timer = ch2.freq_timer_period;
            ch2.dac_out = (ch2.duty >> (ch2.phase)) & 1 ? ch2.envelope_volume : 0.f;
        }
    }


    if (ch3.active) {
    if (!(--ch3.freq_timer)) {
            unsigned char val = mem.read_unsafe(constants::CH3_WAVES_BASE + ch3.wave);
            unsigned char current_val = ch3.nibble == 0 ? (val >> 4) & 0xf : val & 0xf;
            switch ((mem.read_unsafe(constants::NR32_ADDRESS) >> 5) & 0x3) {
                case 0b00: {current_val = 0; break;}
                case 0b01: {break;}
                case 0b10: {current_val>>=1; break;}
                case 0b11: {current_val>>=2; break;}
            }
            ch3.dac_out = ((float)current_val);
            if (ch3.nibble == 1) {ch3.nibble = 0; ch3.wave = (ch3.wave + 1) % 16;}
            else if (ch3.nibble == 0) {ch3.nibble = 1;}
            ch3.freq = ((mem.read_unsafe(constants::NR34_ADDRESS) & 0x7) << 8) | (mem.read_unsafe(constants::NR33_ADDRESS));
            ch3.freq_timer = (2048 - ch3.freq) * 2;
        }
    }

    if (ch4.active) {
        if (!(--ch4.freq_timer)) {
            auto next_bit = (ch4.lsfr & 1) ^ ((ch4.lsfr >> 1) & 1);
            ch4.lsfr >>= 1;
            if (ch4.lsfr_width){
                ch4.lsfr &= 0x7f;
                ch4.lsfr &= ~(1 << 6);
                ch4.lsfr |= (next_bit << 6);
            } else {
                ch4.lsfr &= 0x7fff;
                ch4.lsfr &= ~(1 << 14);
                ch4.lsfr |= (next_bit << 14);
            }
            ch4.dac_out = (float)((!(ch4.lsfr & 1)) * ch4.envelope_volume);
            ch4.freq_timer = ch4.freq_period;
        }
    }

    accum += 44'100;
    if (accum >= 4194304) {
        accum-=4194304;
        auto sample = next_sample_2ch(mem);
        auto left = (highpass<true>(sample.first) * 32767.f);
        auto right = (highpass<false>(sample.second) * 32767.f);
        buffer[std::exchange(current_buffer_i, (current_buffer_i + 1) % constants::SAMPLE_BUFFER_SIZE)] = std::isnan(left) ? 0 : (short)left;
        buffer[std::exchange(current_buffer_i, (current_buffer_i + 1) % constants::SAMPLE_BUFFER_SIZE)] = std::isnan(right) ? 0 : (short)right;
    }
}