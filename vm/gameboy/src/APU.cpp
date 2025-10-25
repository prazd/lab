#include "APU.h"

void CH1::update_duty(const Memory& mem) {
    switch ((mem.read_unsafe(constants::NR11_ADDRESS) >> 6)) {
        case 0b00: {duty = 1/8.f; break;}
        case 0b01: {duty = 1/4.f;  break;}
        case 0b10: {duty = 1/2.f;  break;}
        case 0b11: {duty = 3/4.f;  break;}
    }
}

void CH1::update_pitch(const Memory& mem) {
    unsigned char l = mem.read_unsafe(constants::NR13_ADDRESS);
    unsigned char h = (mem.read_unsafe(constants::NR14_ADDRESS) & 0x7);
    auto x = ((h << 8) | l);
    if (x == 2048)
        return;

    shadow_frequency = x;

    // f = 131072 / (2048 - X) Hz
    pitch = 131072 / (2048 - x);
}

void CH1::init(const Memory& mem) {
    update_pitch(mem);
    length_counter = 64 - (mem.read_unsafe(constants::NR11_ADDRESS) & 0x3f);
}

void CH1::disable_check(Memory &mem) {
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
            // envelope
            break;
        }
        default: break;
    }
    ch1.disable_check(mem);
}


static float next_square_sample(float pitch, float duty){
    // todo:
    return 1.f;
}

void APU::tick(Memory& mem) {
    static int current = 0;
    static unsigned char prev_frame_seq_step = 0;
    auto master_control =  mem.read_unsafe(constants::NR52_ADDRESS);
    // todo: check
    ch1.active = master_control & 1;
//    ch2.active = (master_control >> 1) & 1;
//    ch3.active = (master_control >> 2) & 1;
//    ch4.active = (master_control >> 3) & 1;

    if (!ch1.active && master_control) {
        ch1.init(mem);
        ch1.active = true;
    }

    if (frame_sequencer.step != prev_frame_seq_step) {
        fram_seq_handler(mem);
        prev_frame_seq_step = frame_sequencer.step;
        ch1.update_duty(mem);
        float mix = (next_square_sample(ch1.pitch, ch1.duty));
        if (ch1.active) {
            if (mix > 1.f) {
                mix = 1.f;
            } else if (mix < -1.f) {
                mix = -1.f;
            }
        }
        buffer[current = (current + 1) % 546] = mix * 32767;
    }
}