#include "PPU.hpp"
#include "Constants.hpp"

void PPU::Memory::write(unsigned short addr, unsigned char value) {
    adjust_if_mirror(addr);
    map[addr] = value;
}

unsigned char PPU::Memory::read(unsigned short addr) const {
    adjust_if_mirror(addr);
    return map[addr];
}

void PPU::Memory::adjust_if_mirror(unsigned short& addr) {
    // TODO: check palettes
    if (addr >= constants::PALETTES_MIRROR_BEGIN && addr <= constants::PALETTES_MIRROR_END){
        addr = constants::IMAGE_PALETTE_BEGIN + (addr % 32);
    } else if (addr >= constants::TABLES_MIRROR_BEGIN && addr <= constants::TABLES_MIRROR_END){
        addr = constants::NAME_TABLE0_BEGIN + (addr - constants::TABLES_MIRROR_BEGIN);
    }
}

void PPU::read_ppuctrl(Bus& bus) {
    ppuctrl.set(bus.ppuctrl.reg);
    switch (ppuctrl.base_name_table_addr) {
        case 0b00 : base_name_table_address = constants::NAME_TABLE0_BEGIN; break;
        case 0b10 : base_name_table_address = constants::NAME_TABLE1_BEGIN; break;
        case 0b01 : base_name_table_address = constants::NAME_TABLE2_BEGIN; break;
        default:    base_name_table_address = constants::NAME_TABLE3_BEGIN; break;
    }
    address_increment_on_read_ppudata = ppuctrl.addr_inc;
    sprite_pattern_table_address8x8 = ppuctrl.pattern_table_addr8x8 ? constants::PATTERN_TABLE0_BEGIN : constants::PATTERN_TABLE1_BEGIN;
    sprite_mode = ppuctrl.sprite_sz ?  m8x16 : m8x8;
    // TODO: check
    //    PPU master/slave select
    //    (0: read backdrop from EXT pins; 1: output color on EXT pins)
    vblank_nmi_enable = ppuctrl.vblank_nmi_enable;
}

void PPU::read_ppuscroll(Bus& bus) {
    // TODO: finish
    if (w)
        y_scroll = bus.ppuscroll.reg;
    else
        x_scroll = bus.ppuscroll.reg;
    w = !w;
}

void PPU::read_ppuaddr(Bus& mem) {
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


void PPU::update_state(Bus& bus) {
    if (bus.ppuctrl.was_written)    {read_ppuctrl(bus); bus.ppuctrl.was_written = false;}
    if (bus.ppumask.was_written)    {ppumask.set(bus.ppumask.reg); bus.ppumask.was_written = false;}
    if (bus.ppuscroll.was_written)  {read_ppuscroll(bus); bus.ppuscroll.was_written = false;}
    if (bus.ppuaddr.was_written)    {read_ppuaddr(bus); bus.ppuaddr.was_written = false;}
    // TODO: finish
    if (bus.ppudata.was_written)    {v += ppuctrl.addr_inc ? 1 : 32; bus.ppudata.was_written = false;}
}
