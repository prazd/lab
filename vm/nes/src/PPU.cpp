#include "PPU.h"
#include "Constants.h"

void PPU::read_ppuctrl(Memory &mem) {
    ppuctrl.reg = mem.ppuctrl.reg;
    switch (ppuctrl.fields.base_name_table_addr) {
        case 0b00 : base_name_table_address = constants::NAME_TABLE0_BEGIN; break;
        case 0b10 : base_name_table_address = constants::NAME_TABLE1_BEGIN; break;
        case 0b01 : base_name_table_address = constants::NAME_TABLE2_BEGIN; break;
        default:    base_name_table_address = constants::NAME_TABLE3_BEGIN; break;
    }
    address_increment_on_read_ppudata = ppuctrl.fields.addr_inc;
    sprite_pattern_table_address8x8 = ppuctrl.fields.pattern_table_addr8x8 ? constants::PATTERN_TABLE0_BEGIN : constants::PATTERN_TABLE1_BEGIN;
    sprite_mode = ppuctrl.fields.sprite_sz ?  m8x16 : m8x8;
    // TODO: check
    //    PPU master/slave select
    //    (0: read backdrop from EXT pins; 1: output color on EXT pins)
    vblank_nmi_enable = ppuctrl.fields.vblank_nmi_enable;
}

void PPU::read_ppuscroll(Memory &mem) {
    // TODO: finish
    if (w)
        y_scroll = mem.ppuscroll.reg;
    else
        x_scroll = mem.ppuscroll.reg;
    w = !w;
}

void PPU::read_ppuaddr(Memory &mem) {
    if (w){
        t &= 0xff00;
        t |= mem.ppuaddr.reg;
    } else {
        t &= 0xff;
        t |= ((mem.ppuaddr.reg) << 8); //TODO: maybe 6 byte
    }
    if (w) v = t;
    w = !w;
}


void PPU::update_state(Memory& mem) {
    if (mem.ppuctrl.was_written)    {read_ppuctrl(mem); mem.ppuctrl.was_written = false;}
    if (mem.ppumask.was_written)    {ppumask.reg = mem.ppumask.reg; mem.ppumask.was_written = false;}
    if (mem.ppustatus.was_written)  {ppustatus.reg = mem.ppumask.reg; mem.ppustatus.was_written = false;}
    if (mem.ppuscroll.was_written)  {read_ppuscroll(mem); mem.ppuscroll.was_written = false;}
    if (mem.ppuaddr.was_written)    {read_ppuaddr(mem); mem.ppuaddr.was_written = false;}
}
