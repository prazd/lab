#include "PPU.hpp"
#include "Constants.hpp"
#include <bitset>
#include <cassert>

void PPU::Memory::write(Bus& bus, unsigned short addr, unsigned char value) {
    adjust_if_mirror(addr);
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        bus.mapper.write_chr_ram(addr, value);
        return;
    }
    map[addr] = value;
}

unsigned char PPU::Memory::read(Bus& bus, unsigned short addr) const {
    adjust_if_mirror(addr);
    if (addr >= 0x0000 && addr <= 0x1FFF){
        return bus.read_chr(addr);
    }
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
    if (!w){
        // set x coarse + x
        t &= 0xffe0;
        t |= ((bus.ppuscroll.reg >> 3) & 0x1f);
        x = bus.ppuscroll.reg & 0x7;
    } else {
        // set y coarse + y fine
        t &= 0xfc1f;
        t |= ((bus.ppuscroll.reg & 0xf8)) << 2;

        t &= 0x1fff;
        t |= (t & 0x7) << 12;
    }
    w = !w;
}

void PPU::read_ppuaddr(Bus& bus) {
    if (w){
        t &= 0xff00;
        t |= bus.ppuaddr.reg;
    } else {
        t &= 0xff;
        t |= (((bus.ppuaddr.reg & 0x7f) << 8));
    }
    if (w){
        v = t;
        bus.last_ppu_address = v;
    }
    w = !w;
}

void PPU::update_state(Bus& bus) {
    if (bus.ppustatus.was_written)  {
        w = false;
        bus.ppustatus.was_written = false;}
    if (bus.ppuctrl.was_written)    {read_ppuctrl(bus); bus.ppuctrl.was_written = false;}
    if (bus.ppumask.was_written)    {ppumask.set(bus.ppumask.reg); bus.ppumask.was_written = false;}
    if (bus.ppuscroll.was_written)  {read_ppuscroll(bus); bus.ppuscroll.was_written = false;}
    if (bus.ppuaddr.was_written)    {read_ppuaddr(bus); bus.ppuaddr.was_written = false;}
    // TODO: finish
    if (bus.ppudata.was_written)    {
        unsigned short addr = v;
        Memory::adjust_if_mirror(addr);
        if (addr >= 0x0 && addr <= 0x1FFF){
            mem.write(bus, addr, bus.ppudata.reg);
        } else {
            mem.map[addr] = bus.ppudata.reg;
        }
        v = (v + (ppuctrl.addr_inc ? 32 : 1)) & 0x3fff;
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
    unsigned short base = (addr >> 12) & 1 ? constants::PATTERN_TABLE1_BEGIN : constants::PATTERN_TABLE0_BEGIN;

    return mem.read(bus, base + (tile_number * 16) + y_offset + (bit_plane ? 8 : 0));
}

unsigned short PPU::to_pattern_addr(unsigned char y, unsigned char tile_n, bool plane, bool h) {
    return y | (plane << 3) | (tile_n << 4) | (h << 12);
}

void PPU::reset() {
    current_y = 261;
    current_x = 0;
    current_cycle = 0;
    v = 0;
    t = 0;
    x = 0;
    w = false;
}

void PPU::update_base_nt(Bus& bus) {
    using SM = Mapper::ScreenMirroring;
    unsigned char current_nt_value = ((v >> 10) & 0x3);
    switch (bus.mapper.current_screen_mirroring) {
        case SM::Horizontal: {
            if (current_nt_value == 0 || current_nt_value == 2)
                base_name_table_address = constants::NAME_TABLE0_BEGIN;
            else
                base_name_table_address =  constants::NAME_TABLE1_BEGIN;
            break;
        }
        case SM::Vertical: {
            if (current_nt_value == 0 || current_nt_value == 1)
                base_name_table_address = constants::NAME_TABLE0_BEGIN;
            else
                base_name_table_address = base_name_table_address = constants::NAME_TABLE1_BEGIN;
            break;
        }
        case SM::SingleScreenLower: {base_name_table_address = constants::NAME_TABLE0_BEGIN; break;}
        case SM::SingleScreenUpper: {base_name_table_address = constants::NAME_TABLE1_BEGIN; break;}
        default:
            base_name_table_address = constants::NAME_TABLE0_BEGIN; break;
    }
}

void PPU::tick(Bus& bus) {
    static unsigned short current_scanline_cycle = 0;
    ++current_cycle;
    // TODO: continue debug
    update_state(bus);

    if (current_y < 240){
        static unsigned char nt_byte = 0;
        static unsigned char attrib_value = 0;
        static unsigned char pattern_table_l = 0;
        static unsigned char pattern_table_h = 0;

        if (!current_scanline_cycle){
#if UI_DEBUGGER
            frame_start = false;
#endif
            ++current_scanline_cycle;
            return;}

        // 1-256 cycles
        if (current_scanline_cycle >= 1 && current_scanline_cycle < 257 && ppumask.enable_background_rendering) {
            if (current_scanline_cycle & 1) {
                unsigned char x_tile_n = (v & 0x1f);
                unsigned char y_tile_n = ((v >> 5) & 0x1f);
                update_base_nt(bus);
                switch (current_step) {
                    // fetch nametable byte
                    // TODO: debug base_name_table_address
                    case NT:  { nt_byte = mem.read(bus,  base_name_table_address + (y_tile_n * 32) + x_tile_n); current_step = AT;  break;} // 42
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
                        pattern_table_l = tile_byte_from_addr(bus, to_pattern_addr(((v >> 12) & 0x7), nt_byte, false, ppuctrl.bg_pattern_table_addr));
                        current_step = PTH; break;}     // pattern table tile low
                    case PTH: {
                        pattern_table_h = tile_byte_from_addr(bus, to_pattern_addr(((v >> 12) & 0x7), nt_byte, true, ppuctrl.bg_pattern_table_addr));
                        shift_register_low &= 0xff00;
                        shift_register_low |=  pattern_table_l;
                        shift_register_high &= 0xff00;
                        shift_register_high |= pattern_table_h;

                        unsigned char add_val = (((v & 0x1f) + 1) % 32);
                        if (!add_val) v ^= 0x400;
                        v &= 0xffe0;
                        v |= add_val;

                        attributes_low &= 0xf0;
                        attributes_high &= 0xf0;

                        attributes_low |= (attrib_value & 0x1) ? 0xf : 0;
                        attributes_high |= (attrib_value & 0x2) ? 0xf : 0;

                        current_step = NT;  break;}  // pattern table tile high
                }
            }

            unsigned char pixel_index =
                    mem.read(bus, constants::IMAGE_PALETTE_BEGIN + (4 * ((((attributes_high >> 15) & 1) << 1) | (attributes_low >> 15) & 1)) + ((shift_register_high >> (15-x)) & 1) | ((shift_register_low >> (15-x)) & 1));
            update_internal_display(palette[pixel_index]);

            shift_register_high <<= 1;
            shift_register_low <<= 1;
            attributes_low <<= 1;
            attributes_high <<= 1;

            if (current_scanline_cycle == 256){
                unsigned char y_fine_add_val = ((((v >> 12) & 0x7) + 1) % 8);
                v &= 0x8fff;
                v |= y_fine_add_val << 12;

                if (!y_fine_add_val){
                    auto add_val = ((((v >> 5) & 0x1f) + 1) % 30);
                    if (!add_val) v ^= 0x800;
                    v &= 0xfc1f;
                    v |= (add_val << 5);
                }
            }
        } else if(current_scanline_cycle >= 257 && current_scanline_cycle < 320) {
            if (current_scanline_cycle == 257 && ppumask.enable_background_rendering) {
                // TODO: debug (oam_read test (v & 0x1f) == 2)
                // set coarse x + update ntx bit
                v &= 0xffe0;
                v |= (t & 0x1f);
                v &= 0xfbff;
                v |= (t & (~0xfbff & 0xffff));
            }
        }
        if (current_scanline_cycle++ % 341 == 0){
            ++current_y;
            current_scanline_cycle = 0;
        }
    } else if (current_y >= 240 && current_y < 261){
        if (current_y == 241 && current_scanline_cycle % 341 == 2) {
            ppustatus.vblank_flag = 1;
            bus.ppustatus.reg |= 1 << 7;
            bus.nmi_interrupt_flip_flop = true;
        }
        ++current_scanline_cycle;
        if (current_scanline_cycle % 341 == 0){
            ++current_y;
            current_scanline_cycle = 0;
        }
    } else if (current_y == 261){
        if (current_scanline_cycle++ % 341 == 0){
            // update shift registers
            ppustatus.vblank_flag = 0;
            bus.ppustatus.reg &= 0x7f;
            bus.nmi_interrupt_flip_flop = false; // todo: check

            current_y = 0;
            current_x = 0;
            if (ppumask.enable_background_rendering){
                std::copy(internal_display.begin(), internal_display.end(), display.begin());
                v &= 0x841f;
                v |= ((t & (~0x841f & 0xffff)));

                auto get_next_shifts = [&]() -> std::pair<unsigned short, unsigned char>{
                    unsigned char x_tile_n = (v & 0x1f) + 1;
                    unsigned char y_tile_n = ((v >> 5) & 0x1f);
                    auto nt_byte = mem.read(bus,  constants::NAME_TABLE0_BEGIN + (y_tile_n * 32) + x_tile_n);
                    auto val = mem.read(bus, base_attrib_address + y_tile_n/4 + x_tile_n/4);
                    auto shift_y = (y_tile_n/2) % 2;
                    auto shift_x = (x_tile_n/2) % 2;
                    auto attrib_value = (val >> (2 * ((shift_y * 2) + shift_x))) & 0x3;
                    auto pattern_table_l = tile_byte_from_addr(bus, to_pattern_addr(((v >> 12) & 0x7), nt_byte, false, ppuctrl.bg_pattern_table_addr));
                    auto pattern_table_h = tile_byte_from_addr(bus, to_pattern_addr(((v >> 12) & 0x7), nt_byte, true, ppuctrl.bg_pattern_table_addr));
                    unsigned char add_val = (((v & 0x1f) + 1) % 32);
                    if (!add_val)
                        v ^= 0x400;
                    v &= 0xffe0;
                    v |= add_val;
                    return {((pattern_table_h << 8) | pattern_table_l), attrib_value};
                };
                auto h = get_next_shifts();
                shift_register_low |= (h.first & 0xff) << 8;
                shift_register_high |= h.first & 0xff00;
                attributes_low &= 0xff;
                attributes_high &= 0xff;
                attributes_low |= (h.second & 0x1) ? 0xff00 : 0;
                attributes_high |= (h.second & 0x2) ? 0xff00 : 0;

                auto l = get_next_shifts();
                shift_register_low |= (l.first & 0xff);
                shift_register_high |= ((l.first >> 8)& 0xff);
                attributes_low &= 0xff00;
                attributes_high &= 0xff00;
                attributes_low |= (h.second & 0x1) ? 0xff : 0;
                attributes_high |= (h.second & 0x2) ? 0xff : 0;
                current_scanline_cycle = 0;
#if UI_DEBUGGER
                frame_start = true;
                ++frames_count;
#endif
            }
        }
    }
}