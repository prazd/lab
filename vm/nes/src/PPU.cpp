#include "PPU.hpp"
#include "Constants.hpp"

void PPU::Memory::write(Bus& bus, unsigned short addr, unsigned char value) {
    adjust_if_mirror(addr);
    map[addr] = value;
}

unsigned char PPU::Memory::read(Bus& bus, unsigned short addr) const {
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
    bg_pattern_table_address = ppuctrl.bg_pattern_table_addr ? constants::PATTERN_TABLE0_BEGIN : constants::PATTERN_TABLE1_BEGIN;


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
    if (w)
        v = t;
    w = !w;
}

void PPU::update_state(Bus& bus) {
    if (bus.ppuctrl.was_written)    {read_ppuctrl(bus); bus.ppuctrl.was_written = false;}
    if (bus.ppumask.was_written)    {ppumask.set(bus.ppumask.reg); bus.ppumask.was_written = false;}
    if (bus.ppuscroll.was_written)  {read_ppuscroll(bus); bus.ppuscroll.was_written = false;}
    if (bus.ppuaddr.was_written)    {read_ppuaddr(bus); bus.ppuaddr.was_written = false;}
    // TODO: finish
    if (bus.ppudata.was_written)    {
        unsigned short addr = v;
        Memory::adjust_if_mirror(addr);
        mem.map[addr] = bus.ppudata.reg;
        v =  (v + (ppuctrl.addr_inc ? 32 : 1)) & 0x3fff; // todo: switch
        bus.ppudata.was_written = false;
    }
}

unsigned char PPU::tile_byte_from_addr(Bus& bus, unsigned short addr) {
//    DCBA98 76543210
//    ---------------
//    0HNNNN NNNNPyyy
//    |||||| |||||+++- T: Fine Y offset, the row number within a tile
//    |||||| ||||+---- P: Bit plane (0: less significant bit; 1: more significant bit)
//    ||++++-++++----- N: Tile number from name table
//    |+-------------- H: Half of pattern table (0: "left"; 1: "right")
//    +--------------- 0: Pattern table is at $0000-$1FFF
    unsigned char y_offset = addr & 0x7;
    bool bit_plane = (addr >> 3) & 1;
    unsigned char tile_number = (addr >> 4) & 0xff;
    unsigned short base = (addr >> 12) & 1 ? constants::PATTERN_TABLE0_BEGIN : constants::PATTERN_TABLE1_BEGIN;

    return mem.read(bus, base + (tile_number * 16) + y_offset + (bit_plane ? 8 : 0));
}

unsigned short PPU::to_pattern_addr(unsigned char y, unsigned char tile_n, bool plane, bool h) {
    return y | (plane << 3) | (tile_n << 4) | (h << 12);
}

void PPU::reset() {
    current_y = 0;
    current_x = 0;
    current_cycle = 0;
    v = 0;
    t = 0;
    x = 0;
}

void PPU::tick(Bus& bus) {
    ++current_cycle;
    // TODO: continue debug
    update_state(bus);
    if (!ppumask.enable_background_rendering) return;
    if (current_y == 0) {
        ++current_y;
        return;
    } // idle cycle

    if (current_y < 240){
        static unsigned char nt_byte = 0;
        static unsigned char attrib_value = 0;
        static unsigned char pattern_table_l = 0;
        static unsigned char pattern_table_h = 0;

        unsigned char x_tile_n = (v & 0x1f);
        unsigned char y_tile_n = ((v >> 5) & 0x1f);

        // 1-256 cycles
        if (current_step % 2 == 0) {
            switch (current_step) {
                // fetch nametable byte
                case NT:  { nt_byte = mem.read(bus, base_name_table_address + (y_tile_n * 32) + x_tile_n); current_step = AT;  break;}
                case AT:  {
                    auto val = mem.read(bus, base_attrib_address + y_tile_n/4 + x_tile_n/4);
                    // TODO: check
                    //  3 3 2 2
                    //  3 3 2 2
                    //  1 1 0 0
                    //  1 1 0 0
                    auto shift_y = (y_tile_n/2) % 2;
                    auto shift_x = (x_tile_n/2) % 2;
                    attrib_value = (val >> (2 * ((shift_y * 2) + shift_x))) & 0x3;
                    current_step = PTL;
                    break;}  // fetch attribute table byte
                case PTL: {
                    pattern_table_l = tile_byte_from_addr(bus, to_pattern_addr(y_tile_n % 8, nt_byte, false, ppuctrl.bg_pattern_table_addr));
                    current_step = PTH; break;}     // pattern table tile low
                case PTH: {
                    pattern_table_h = tile_byte_from_addr(bus, to_pattern_addr(y_tile_n % 8, nt_byte, true, ppuctrl.bg_pattern_table_addr));
                    shift_register_low |= pattern_table_l;
                    shift_register_high |= pattern_table_h;
                    current_step = NT;  break;}     // pattern table tile high
            }
        }
        // Get pixel
        shift_register_high <<= 1;
        shift_register_low <<= 1;
        //   TODO: maybe use register for attrib_value
        unsigned char pixel_index =
                mem.read(bus, constants::IMAGE_PALETTE_BEGIN + (4 * attrib_value) + ((bool(shift_register_high & 0x8000) << 1) | bool(shift_register_low & 0x8000)));
        update_internal_display(palette[pixel_index]);
        if (pixel_index != 0)
            pixel_index += 0;
        if (current_x % 256 == 0){
            ++current_y;
        }
    } else if (current_y == 240){
        if (current_x++ % 256 == 0){
            ++current_y;
        }
    } else if (current_y > 240 && current_y < 261){
        if (current_y == 241 && (current_x % 256) == 1){
            // VBlank
        } else if (current_x % 256 == 0){
            ++current_y;
        }
        ++current_x;
    } else if (current_y == 261){
        //
        current_y = 0;
        current_x = 0;
        std::copy(internal_display.begin(), internal_display.end(), display.begin());
    }
}
