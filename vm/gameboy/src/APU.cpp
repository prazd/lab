#include "APU.h"

void SquareWaveChannel::update_duty(const Memory& mem, unsigned short address) {
    switch ((mem.read_unsafe(address) >> 6)) {
        case 0b00: {duty = 0b0000'0001;  break;} // 12.5%
        case 0b01: {duty = 0b1000'0011;  break;} // 25%
        case 0b10: {duty = 0b0000'1111;  break;} // 50%
        case 0b11: {duty = 0b1111'1110;  break;} // 75%
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

void CH1::init(const Memory& mem) {
    update_duty(mem, constants::NR11_ADDRESS);
    update_pitch(mem, constants::NR13_ADDRESS, constants::NR14_ADDRESS);
    phase = 0;
    length_counter = 64 - (mem.read_unsafe(constants::NR11_ADDRESS) & 0x3f);
    freq_timer = freq_timer_period;
    envelope_volume = (mem.read_unsafe(constants::NR12_ADDRESS) >> 4) & 0xf;
    envelope_timer = (mem.read_unsafe(constants::NR12_ADDRESS) & 0x7);
    sweep_timer = ((mem.read_unsafe(constants::NR10_ADDRESS) >> 4) & 0x7);
    if (sweep_timer == 0)
        sweep_timer = 8;
}

void CH2::init(const Memory& mem) {
    update_duty(mem, constants::NR21_ADDRESS);
    update_pitch(mem, constants::NR23_ADDRESS, constants::NR24_ADDRESS);
    phase = 0;
    length_counter = 64 - (mem.read_unsafe(constants::NR21_ADDRESS) & 0x3f);
    freq_timer = freq_timer_period;
    envelope_volume = (mem.read_unsafe(constants::NR22_ADDRESS) >> 4) & 0xf;
    envelope_timer = (mem.read_unsafe(constants::NR22_ADDRESS) & 0x7);
    if (envelope_timer == 0) envelope_timer = 8;
}

void SquareWaveChannel::disable_check(Memory& mem) {
    if (length_counter == 0 && (mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1) {
        // TODO: disable channel
    }
}

void CH1::sweep_step(Memory &mem) {
    auto nr10 = mem.read_unsafe(constants::NR10_ADDRESS);
    bool neg = (nr10 >> 3) & 1;
    unsigned char shift = nr10 & 0x7;
    auto delta = shadow_frequency >> shift;
    unsigned new_val{};
    if (!neg) new_val = shadow_frequency + delta;
    else new_val = shadow_frequency - delta;
    if (new_val > 2047){
        active = false;
    }
    shadow_frequency = new_val;
    mem.write_unsafe(constants::NR13_ADDRESS, new_val & 0xff);
    mem.write_unsafe(constants::NR14_ADDRESS,
                     (mem.read_unsafe(constants::NR14_ADDRESS) & 0xf8) | ((new_val >> 8) & 0x7));
    sweep_timer = ((mem.read_unsafe(constants::NR10_ADDRESS) >> 4) & 0x7);
    update_pitch(mem, constants::NR13_ADDRESS, constants::NR14_ADDRESS);
}

void CH3::update_length_timer(const Memory &mem) {
    length_counter = (mem.read_unsafe(constants::NR31_ADDRESS));
}

void CH4::update_length_timer(const Memory &mem) {
    length_counter = (mem.read_unsafe(constants::NR41_ADDRESS) & 0x3f);
}

void FrameSequencer::tick(Memory& mem) {
    static auto prev = 0;
    unsigned char current = 0;
    prev = std::exchange(current, (mem.read_unsafe(constants::DIV_REGISTER) >> 4) & 1);
    if (prev && !current)
        step = (step + 1) % 8;
}


void APU::frame_seq_handler(Memory& mem) {
    switch (frame_sequencer.step) {
        case 0: {
            if ((mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1) --ch1.length_counter;
            break;
        }
        case 2: {
            if ((mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1) --ch1.length_counter;
            if (!ch1.sweep_timer) ch1.sweep_step(mem);
            else --ch1.sweep_timer;
            break;
        }
        case 4: {
            if ((mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1)  --ch1.length_counter;
            break;
        }
        case 6: {
            if ((mem.read_unsafe(constants::NR14_ADDRESS) >> 6) & 1) --ch1.length_counter;
            if (!ch1.sweep_timer) ch1.sweep_step(mem);
            else --ch1.sweep_timer;
            break;
        }
        case 7: {
            if (!ch1.envelope_timer) {
                if ((mem.read_unsafe(constants::NR12_ADDRESS) >> 3) & 1) {
                    ch1.envelope_volume += ch1.envelope_volume == 15 ? 0 : 1;
                } else {
                    ch1.envelope_volume -= ch1.envelope_volume == 0 ? 0 : 1;
                }
                ch1.envelope_timer = (mem.read_unsafe(constants::NR12_ADDRESS) & 0x7);
            }
            --ch1.envelope_timer;

            if (!ch2.envelope_timer) {
                if ((mem.read_unsafe(constants::NR22_ADDRESS) >> 3) & 1) {
                    ch2.envelope_volume += ch2.envelope_volume == 15 ? 0 : 1;
                } else {
                    ch2.envelope_volume -= ch2.envelope_volume == 0 ? 0 : 1;
                }
                ch2.envelope_timer = (mem.read_unsafe(constants::NR22_ADDRESS) & 0x7);
            }
            --ch2.envelope_timer;
            break;
        }
        default: break;
    }
    ch1.disable_check(mem);
}

static float next_square_sample(float dac_value){
    return (dac_value/7.5) - 1.f;
}

void APU::tick(Memory& mem) {
    static unsigned t_cycle = 0;
    static int current = 0;
    static unsigned char prev_frame_seq_step = 0;
    static bool prev_trigger = false;
    auto master_control =  mem.read_unsafe(constants::NR52_ADDRESS);

    if (!prev_trigger && ((mem.read_unsafe(constants::NR14_ADDRESS) >> 7) & 1)
         && (mem.read_unsafe(constants::NR12_ADDRESS) & 0xf8) != 0) {
        ch1.init(mem);
        ch1.active = true;
    }

    if (!prev_trigger && ((mem.read_unsafe(constants::NR24_ADDRESS) >> 7) & 1)
        && (mem.read_unsafe(constants::NR12_ADDRESS) & 0xf8) != 0) {
        ch2.init(mem);
        ch2.active = true;
    }

    if (ch1.active) {
        if (!(--ch1.freq_timer)) {
            ch1.phase = (ch1.phase + 1) % 8;
            ch1.freq_timer = ch1.freq_timer_period;
        }
    }

    if (ch2.active) {
        if (!(--ch2.freq_timer)) {
            ch2.phase = (ch2.phase + 1) % 8;
            ch2.freq_timer = ch2.freq_timer_period;
        }
    }

    if (t_cycle++ % 95 == 0) {
        float ch1_res = (next_square_sample((ch1.duty >> ch1.phase) & 1 ? ch1.envelope_volume : 0));
        float ch2_res = (next_square_sample((ch2.duty >> ch2.phase) & 1 ? ch2.envelope_volume : 0));
        auto mix = (ch2_res + ch1_res)/2;
        if (mix > 1.f) {
            mix = 1.f;
        } else if (mix < -1.f) {
            mix = -1.f;
        }
        buffer[current = (current + 1) % 750] = (short)(mix * 32767.f);
    }

    if (frame_sequencer.step != prev_frame_seq_step) {
        frame_seq_handler(mem);
        prev_frame_seq_step = frame_sequencer.step;
    }
    prev_trigger = (mem.read_unsafe(constants::NR14_ADDRESS) >> 7) & 1;
}