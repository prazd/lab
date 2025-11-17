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
    if (!length_counter)
        length_counter = 64 - (mem.read_unsafe(constants::NR11_ADDRESS) & 0x3f);
    freq_timer = freq_timer_period;
    envelope_volume = (mem.read_unsafe(constants::NR12_ADDRESS) >> 4) & 0xf;
    envelope_timer = (mem.read_unsafe(constants::NR12_ADDRESS) & 0x7);
    if (!envelope_timer) envelope_timer = 8;
    mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) | 1);
}


void CH2::init(const Memory& mem) {
    update_duty(mem, constants::NR21_ADDRESS);
    update_pitch(mem, constants::NR23_ADDRESS, constants::NR24_ADDRESS);
    phase = 0;
    if (!length_counter)
        length_counter = 64 - (mem.read_unsafe(constants::NR21_ADDRESS) & 0x3f);
    freq_timer = freq_timer_period;
    envelope_volume = (mem.read_unsafe(constants::NR22_ADDRESS) >> 4) & 0xf;
    envelope_timer = (mem.read_unsafe(constants::NR22_ADDRESS) & 0x7);
    if (!envelope_timer) envelope_timer = 8;
}


void CH3::init(Memory& mem) {
//    nibble = 0;
//    wave = 0;
    freq = ((mem.read_unsafe(constants::NR34_ADDRESS) & 0x7) << 8) | (mem.read_unsafe(constants::NR33_ADDRESS));
    length_enabled = (mem.read_unsafe(constants::NR34_ADDRESS) >> 6) & 1;
    if (length_enabled) length_counter = mem.read_unsafe(constants::NR31_ADDRESS);
    freq_timer = (2048 - freq) * 2;
}

void CH4::init(const Memory& mem) {
    length_enabled = (mem.read_unsafe(constants::NR34_ADDRESS) >> 6) & 1;
    if (length_enabled)
        length_counter = length_counter == 0 ? 64 - (mem.read_unsafe(constants::NR41_ADDRESS) & 0x3f) : length_counter;
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
    mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) & 0xE);
    envelope_volume = 0;
    sweep_timer = 0;
    active = false;
    dac_out = 0.f;
}

void CH2::disable(Memory& mem) {
    mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) & 0xD);
    envelope_volume = 0;
    active = false;
    dac_out = 0.f;
}

void CH3::disable(Memory &mem) {
    mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) & 0xB);
    length_enabled = false;
    length_counter = 0;
    dac_out = 0.f;
    active = false;
}

void CH4::disable(Memory &mem) {
    mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) & 0x7);
    length_enabled = false;
    envelope_volume = 0;
    active = false;
    freq_timer = 0;
}

void CH1::disable_check(Memory& mem) {
    if ((length_counter == 0 && (mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1)) disable(mem);
}

void CH2::disable_check(Memory& mem) {
    if ((length_counter == 0 && (mem.read_unsafe(constants::NR24_ADDRESS) >> 6) & 1)
    || !((mem.read_unsafe(constants::NR22_ADDRESS) >> 4) & 0xf))  disable(mem);
}

void CH3::disable_check(Memory &mem) {
    if (active && length_enabled && length_counter == 0) disable(mem);
}

void CH4::disable_check(Memory &mem) {
    if (active && length_enabled && length_counter == 0) disable(mem);
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
    // todo: maybe second overflow check
    update_pitch(mem, constants::NR13_ADDRESS, constants::NR14_ADDRESS);
}

void APU::frame_seq_tick(Memory& mem) {
    if (frame_seq_t_cycle++ % 8192 == 0)
        frame_seq_handler(mem, std::exchange(frame_sequencer_step, (frame_sequencer_step + 1) % 8));
}

void APU::frame_seq_handler(Memory& mem, unsigned char step) {
    switch (step) {
        case 0: {
            if (ch1.active && (mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1)  --ch1.length_counter;
            if (ch2.active && (mem.read_unsafe(constants::NR24_ADDRESS) >> 6) & 1)  --ch2.length_counter;
            if (ch3.active && ch3.length_enabled) --ch3.length_counter;
            if (ch4.active && (mem.read_unsafe(constants::NR44_ADDRESS) >> 6) & 1)  --ch4.length_counter;
            break;
        }
        case 2: {
            if (ch1.active) {
                if ((mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1) --ch1.length_counter;
                if (!(--ch1.sweep_timer)) ch1.sweep_step(mem, false);
            }
            if (ch2.active && (mem.read_unsafe(constants::NR24_ADDRESS) >> 6) & 1)  --ch2.length_counter;
            if (ch3.active && ch3.length_enabled) --ch3.length_counter;
            if (ch4.active && (mem.read_unsafe(constants::NR44_ADDRESS) >> 6) & 1)  --ch4.length_counter;
            break;
        }
        case 4: {
            if (ch1.active && (mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1)  --ch1.length_counter;
            if (ch2.active && (mem.read_unsafe(constants::NR24_ADDRESS) >> 6) & 1)  --ch2.length_counter;
            if (ch3.active && ch3.length_enabled) --ch3.length_counter;
            if (ch4.active && (mem.read_unsafe(constants::NR44_ADDRESS) >> 6) & 1)  --ch4.length_counter;
            break;
        }
        case 6: {
            if (ch1.active){
                if ((mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1) --ch1.length_counter;
                if (!(--ch1.sweep_timer)) ch1.sweep_step(mem, false);
            }
            if (ch2.active && (mem.read_unsafe(constants::NR24_ADDRESS) >> 6) & 1)  --ch2.length_counter;
            if (ch3.active && ch3.length_enabled) --ch3.length_counter;
            if (ch4.active && (mem.read_unsafe(constants::NR44_ADDRESS) >> 6) & 1)  --ch4.length_counter;
            break;
        }
        case 7: {
            --ch1.envelope_timer;
            if (!ch1.envelope_timer) {
                if ((mem.read_unsafe(constants::NR12_ADDRESS) & 0x7))
                    ch1.envelope_volume = std::clamp((mem.read_unsafe(constants::NR12_ADDRESS) >> 3) & 1 ?
                                                     (char)(ch1.envelope_volume + 1) : (char)(ch1.envelope_volume - 1), (char)0, (char)15);
                ch1.envelope_timer = (mem.read_unsafe(constants::NR12_ADDRESS) & 0x7);
            }

            --ch2.envelope_timer;
            if (!ch2.envelope_timer) {
                if ((mem.read_unsafe(constants::NR22_ADDRESS) & 0x7)){
                    ch2.envelope_volume = std::clamp((mem.read_unsafe(constants::NR22_ADDRESS) >> 3) & 1 ?
                                                     (char)(ch2.envelope_volume + 1) : (char)(ch2.envelope_volume - 1), (char)0, (char)15);
                }
                ch2.envelope_timer = (mem.read_unsafe(constants::NR22_ADDRESS) & 0x7);
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

static float next_square_sample(float dac_value){
    return (dac_value/15.f);
}


template <bool left>
float highpass(float x) {
    static float prev_x = 0.f;
    static float prev_y = 0.f;
    static constexpr float alpha = 0.999958f;
    auto result = x - prev_x + (alpha * prev_y);
    prev_x = x;
    prev_y = result;
    return result;
}

#define FILTER 1

void APU::tick(Memory& mem) {
    static unsigned t_cycle = 0;
    static int current = 0;
    static unsigned char prev_frame_seq_step = 0;
    static bool skip_sample = false;


    if (mem.ch1_trigger
         && (mem.read_unsafe(constants::NR12_ADDRESS) & 0xf8) != 0) {
        ch1.init(mem);
        ch1.active = true;
        mem.ch1_trigger = false;

        // sweep overflow check
        ch1.sweep_step(mem, true);
    }

    if (!mem.read_unsafe(constants::NR12_ADDRESS)) ch1.disable(mem);

    if (mem.ch2_trigger
        && (mem.read_unsafe(constants::NR22_ADDRESS) & 0xf8) != 0) {
        mem.write_unsafe(constants::NR52_ADDRESS, mem.read_unsafe(constants::NR52_ADDRESS) | 1 << 1);
        ch2.init(mem);
        ch2.active = true;
        mem.ch2_trigger = false;
    }

    if (mem.ch3_trigger) {
        ch3.init(mem);
        ch3.active = true;
        mem.ch3_trigger = false;
    }

    if (mem.ch4_trigger) {
        ch4.init(mem);
        ch4.active = true;
        mem.ch4_trigger = false;
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

    if (ch3.active) {
        if (!((mem.read_unsafe(constants::NR30_ADDRESS) >> 7) & 1)){
            ch3.disable(mem);
        } else if (!(--ch3.freq_timer)) {
            unsigned char val = mem.read_unsafe(constants::CH3_WAVES_BASE + ch3.wave);
            ch3.dac_out = ch3.nibble == 0 ? float((val >> 4) & 0xf) : float(val & 0xf);
            switch ((mem.read_unsafe(constants::NR32_ADDRESS) >> 5) & 0x3) {
                case 0b00: {ch3.dac_out = 0; break;}
                case 0b01: {ch3.dac_out = (ch3.dac_out - 7.5f)/7.5f;       break;}
                case 0b10: {ch3.dac_out = ((ch3.dac_out / 2) - 7.5f)/15.f; break;}
                case 0b11: {ch3.dac_out = ((ch3.dac_out / 4) - 7.5f)/30.f; break;}
            }
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
                ch4.lsfr &= 0x7f'ff;
                ch4.lsfr &= ~(1 << 14);
                ch4.lsfr |= (next_bit << 14);
            }
            ch4.dac_out = (float)((!(ch4.lsfr & 1)) * ch4.envelope_volume);
            ch4.freq_timer = ch4.freq_period;
        }
    }


#if FILTER
    if (t_cycle++ % 12 == 0) {
#else
    if (t_cycle++ % 96 == 0) {
#endif
            static int sample_count = 0;
            static float gmix_left = 0.f;
            static float gmix_right = 0.f;
            auto nr51 = mem.read_unsafe(constants::NR51_ADDRESS);
            auto nr50 = mem.read_unsafe(constants::NR50_ADDRESS);
            auto mix_left = 0.f;
            auto mix_right = 0.f;
            float count = 0.f;
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
                    mix_left += ch1_left_active ? (next_square_sample(ch1.dac_out)) : 0;
                    mix_right += ch1_right_active ? (next_square_sample(ch1.dac_out)) : 0;
                    count += 1.f;
                }
#else
                mix_left += ch1_left_active ? (next_square_sample(ch1.dac_out)) : 0;
                mix_right += ch1_right_active ? (next_square_sample(ch1.dac_out)) : 0;
                count += 1.f;
#endif
            }
            if (ch2.active){
#if DEBUG_MODE
                if (mem.custom_ch2_active){
                    mix_left += ch2_left_active ? (next_square_sample(ch2.dac_out)) : 0;
                    mix_right += ch2_right_active ? (next_square_sample(ch2.dac_out)) : 0;
                    count += 1.f;
                }
#else
                mix_left += ch2_left_active ? (next_square_sample(ch2.dac_out)) : 0;
                mix_right += ch2_right_active ? (next_square_sample(ch2.dac_out)) : 0;
                count += 1.f;
#endif
            }

        if (ch3.active){
#if DEBUG_MODE
            if (mem.custom_ch3_active){
                    mix_left += ch3_left_active ? ch3.dac_out/2 : 0;
                    mix_right += ch3_right_active ? ch3.dac_out/2: 0;
                    count += 1.f;
            }
#else
                mix_left += ch3_left_active ?  ch3.dac_out : 0;
                mix_right += ch3_right_active ? ch3.dac_out : 0;
                count += 1.f;
#endif
        }


        if (ch4.active){
#if DEBUG_MODE
            if (mem.custom_ch4_active){
                mix_left += ch4_left_active ? ch4.dac_out : 0;
                mix_right += ch4_right_active ? ch4.dac_out : 0;
                count += 1.f;
            }
#else
                mix_left += ch4_left_active ?  ch4.dac_out : 0;
                mix_right += ch4_right_active ? ch4.dac_out : 0;
                count += 1.f;
#endif
        }

//            mix_left/=count;
//            mix_right/=count;

            mix_left = std::clamp(mix_left , -1.f, 1.f);
            mix_right = std::clamp(mix_right, -1.f, 1.f);
#if FILTER
            gmix_left+= highpass<true>(mix_left);
            gmix_right+= highpass<false>(mix_right);
            if (sample_count++ == 8){
                buffer[current = (current + 1) % constants::SAMPLE_BUFFER_SIZE] = (short)((gmix_left/16) * 32767.f);
                buffer[current = (current + 1) % constants::SAMPLE_BUFFER_SIZE] = (short)((gmix_right/16) * 32767.f);
                gmix_right = 0.f;
                gmix_left = 0.f;
                sample_count = 0;
            }
#else
        buffer[current = (current + 1) % 2048] = (short)(mix_left * 32767.f);
        buffer[current = (current + 1) % 2048] = (short)(mix_right * 32767.f);
#endif

    }
}