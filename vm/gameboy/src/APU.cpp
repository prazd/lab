#include "APU.h"


void APU::tick(Memory& mem) {
    auto master_control =  mem.read_unsafe(constants::NR52_ADDRESS);
    // todo: check
    ch1.active = master_control & 1;
    ch2.active = (master_control >> 1) & 1;
    ch3.active = (master_control >> 2) & 1;
    ch4.active = (master_control >> 3) & 1;

    // CH1 sweep

}