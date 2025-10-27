#include "APU.h"

void CH1::update_duty(const Memory& mem) {
    switch ((mem.read_unsafe(constants::NR11_ADDRESS) >> 6)) {
        case 0b00: {duty = 0b0000'0001;  break;} // 12.5%
        case 0b01: {duty = 0b1000'0001;  break;} // 25%
        case 0b10: {duty = 0b1000'0111;  break;} // 50%
        case 0b11: {duty = 0b0111'1110;  break;} // 75%
    }
}

void CH1::update_pitch(const Memory& mem) {
    unsigned char l = mem.read_unsafe(constants::NR13_ADDRESS);
    unsigned char h = (mem.read_unsafe(constants::NR14_ADDRESS) & 0x7);
    auto x = ((h << 8) | l);
    if (x == 2048)
        return;

    shadow_frequency = x;

    pitch = 131072 / (2048 - x);  // Hz
    freq_timer = (2048 - x) * 4;
    freq_timer_period = freq_timer;
}

void CH1::init(const Memory& mem) {
    update_pitch(mem);
    phase = 0;
    length_counter = 64 - (mem.read_unsafe(constants::NR11_ADDRESS) & 0x3f);
    freq_timer = freq_timer_period;
    envelope_volume = (mem.read_unsafe(constants::NR12_ADDRESS) >> 4) & 0xf;
    envelope_timer = (mem.read_unsafe(constants::NR12_ADDRESS) & 0x7);
}

void CH1::disable_check(Memory& mem) {
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
    if (new_val > 2047){/*TODO: disable channel*/}
    shadow_frequency = new_val;
    mem.write_unsafe(constants::NR13_ADDRESS, new_val & 0xff);
    mem.write_unsafe(constants::NR14_ADDRESS,
                     (mem.read_unsafe(constants::NR14_ADDRESS) & 0xf8) | ((new_val >> 8) & 0x7));
    pitch = shadow_frequency;
}

void CH2::update_length_timer(const Memory &mem) {
    length_counter = (mem.read_unsafe(constants::NR21_ADDRESS) & 0x3f);
}

void CH3::update_length_timer(const Memory &mem) {
    length_counter = (mem.read_unsafe(constants::NR31_ADDRESS));
}

void CH4::update_length_timer(const Memory &mem) {
    length_counter = (mem.read_unsafe(constants::NR41_ADDRESS) & 0x3f);
}

void FrameSequencer::tick() {
    auto prev = frame_seq_counter;
    frame_seq_counter = (frame_seq_counter + 1) % 8192;
    if (prev > 0 && frame_seq_counter == 0)
        step = (step + 1) % 8;
}


void APU::fram_seq_handler(Memory& mem) {
    switch (frame_sequencer.step) {
        case 0: {
            --ch1.length_counter;
            break;
        }
        case 2: {
            --ch1.length_counter;
            --ch1.sweep_timer;
            if (ch1.sweep_timer) ch1.sweep_step(mem);
            break;
        }
        case 4: {
            --ch1.length_counter;
            break;
        }
        case 6: {
            --ch1.length_counter;
            --ch1.sweep_timer;
            if (ch1.sweep_timer) ch1.sweep_step(mem);
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
    auto master_control =  mem.read_unsafe(constants::NR52_ADDRESS);
    // todo: check
//    ch1.active = master_control & 1;
//    ch2.active = (master_control >> 1) & 1;
//    ch3.active = (master_control >> 2) & 1;
//    ch4.active = (master_control >> 3) & 1;

    if (!ch1.active && master_control & 1) {
        ch1.init(mem);
        ch1.active = true;
    }

    if (!(--ch1.freq_timer)) {
        if ((mem.read_unsafe(constants::NR14_ADDRESS) >> 7) & 1){
            ch1.update_pitch(mem);
            ch1.update_duty(mem);
        }
        ch1.phase = (ch1.phase + 1) % 8;
        ch1.freq_timer = ch1.freq_timer_period;
    }

    // TODO: debug
    if (t_cycle++ % 400 == 0) {
        float mix = (next_square_sample((ch1.duty >> ch1.phase) & 1 ? ch1.envelope_volume : 0));
        if (mix > 1.f) {
            mix = 1.f;
        } else if (mix < -1.f) {
            mix = -1.f;
        }
//        mix *= 0.8f;
        buffer[current = (current + 1) % 1024] = (short)(mix * 32767.f);
    }

    if (frame_sequencer.step != prev_frame_seq_step) {
        fram_seq_handler(mem);
        prev_frame_seq_step = frame_sequencer.step;
    }
}