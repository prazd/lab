#include "PPU.h"
#include "Constants.h"
void PPU::read_ppuctrl(Memory &mem) {
    switch (mem.ppuctrl & 0x3) {
        case 0b00 : base_name_table_address = constants::NAME_TABLE0_BEGIN; break;
        case 0b10 : base_name_table_address = constants::NAME_TABLE1_BEGIN; break;
        case 0b01 : base_name_table_address = constants::NAME_TABLE2_BEGIN; break;
        default:    base_name_table_address = constants::NAME_TABLE3_BEGIN; break;
    }

}