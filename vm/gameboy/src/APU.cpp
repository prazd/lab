#include "APU.h"

void CH1::update_duty(Memory &mem) {
    switch ((mem.read_unsafe(constants::NR11_ADDRESS) >> 6)) {
        case 0b00: {duty = 12.5f; break;}
        case 0b01: {duty = 25.f;  break;}
        case 0b10: {duty = 50.f;  break;}
        case 0b11: {duty = 75.f;  break;}
    }
}

void CH1::update_length_timer(Memory &mem) {
    length_counter = (mem.read_unsafe(constants::NR11_ADDRESS) & 0x3f);
}

void CH2::update_length_timer(Memory &mem) {
    length_counter = (mem.read_unsafe(constants::NR21_ADDRESS) & 0x3f);
}

void CH3::update_length_timer(Memory &mem) {
    length_counter = (mem.read_unsafe(constants::NR31_ADDRESS));
}

void CH4::update_length_timer(Memory &mem) {
    length_counter = (mem.read_unsafe(constants::NR41_ADDRESS) & 0x3f);
}

void FrameSequencer::tick() {
    auto prev = frame_seq_counter;
    frame_seq_counter = (frame_seq_counter + 1) % 8192;
    if (prev > 0 && frame_seq_counter == 0)
        step = (step + 1) % 8;
}

void APU::tick(Memory& mem) {
    auto master_control =  mem.read_unsafe(constants::NR52_ADDRESS);
    // todo: check
    ch1.active = master_control & 1;
    ch2.active = (master_control >> 1) & 1;
    ch3.active = (master_control >> 2) & 1;
    ch4.active = (master_control >> 3) & 1;

}