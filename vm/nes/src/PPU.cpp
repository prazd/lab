#include "PPU.hpp"
#include "Constants.hpp"
#include <algorithm>
#include <bitset>
#include <cassert>

void PPU::Memory::write(Bus& bus, unsigned short addr, unsigned char value) {
    adjust_if_mirror(addr);
#if DEBUG_MODE
    if (std::any_of(write_trigger_addresses.begin(), write_trigger_addresses.end(), [addr](auto val){return val == addr;}))
        triggered_on_write = true;
#endif
    if (addr >= 0x0000 && addr <= 0x1FFF && ((bus.ppustatus.reg >> 7) & 1 || !((bus.ppumask.reg >> 3) & 1) || !((bus.ppumask.reg >> 4) & 1))) {
        bus.mapper.write_chr_ram(addr, value);
        return;
    }
    map[addr] = value;
}

unsigned char PPU::Memory::read(Bus& bus, unsigned short addr) const {
    adjust_if_mirror(addr);
#if DEBUG_MODE
    if (std::any_of(read_trigger_addresses.begin(), read_trigger_addresses.end(), [addr](auto val){return val == addr;}))
        triggered_on_read = true;
#endif
    if (addr >= 0x0000 && addr <= 0x1FFF){
        return bus.read_chr(addr);
    }
    return map[addr];
}

void PPU::Memory::adjust_if_mirror(unsigned short& addr) {
    // TODO: check palettes
    if (addr >= constants::PALETTES_MIRROR_BEGIN && addr <= constants::PALETTES_MIRROR_END)
        addr = constants::IMAGE_PALETTE_BEGIN + (addr % 32);
    else if (addr >= constants::TABLES_MIRROR_BEGIN && addr <= constants::TABLES_MIRROR_END)
        addr = constants::NAME_TABLE0_BEGIN + (addr - constants::TABLES_MIRROR_BEGIN);
    else if (addr == 0x3f10 || addr == 0x3f14 || addr == 0x3f18 || addr == 0x3f1c)
        addr -= 0x10;
}

void PPU::read_ppuctrl(Bus& bus) {
    ppuctrl.set(bus.ppuctrl.reg);
    t &= 0xf3ff;
    t |= (ppuctrl.base_name_table_addr << 10);

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
        t &= 0x7fe0;
        t |= ((bus.ppuscroll.reg >> 3) & 0x1f);
        x = bus.ppuscroll.reg & 0x7;
    } else {
        // set y coarse + y fine
        t &= 0xc1f;
        t |= ((bus.ppuscroll.reg >> 3) & 0x1f) << 5;
        t |= (bus.ppuscroll.reg & 0x7) << 12;
    }
    w = !w;
}

void PPU::read_ppuaddr(Bus& bus) {
    if (w){
        t &= 0x7f00;
        t |= bus.ppuaddr.reg;
    } else {
        t &= 0xff;
        t |= (((bus.ppuaddr.reg & 0x7f) << 8));
    }
    if (w){
        bus.mapper.update_state(t, v);
        v = t;
        bus.last_ppu_address = v;

    }
    w = !w;
}

void PPU::update_state(Bus& bus) {
    if (bus.ppustatus.read_requested)  {
        w = false;
        bus.ppustatus.read_requested = false;}
    if (bus.ppuctrl.write_requested)    {read_ppuctrl(bus); bus.ppuctrl.write_requested = false;}
    if (bus.ppumask.write_requested)    {ppumask.set(bus.ppumask.reg); bus.ppumask.write_requested = false;}
    if (bus.ppuscroll.write_requested)  {read_ppuscroll(bus); bus.ppuscroll.write_requested = false;}
    if (bus.ppuaddr.write_requested)    {read_ppuaddr(bus); bus.ppuaddr.write_requested = false;}
    // TODO: finish
    if (bus.ppudata.write_requested)   {
        unsigned short addr = v;
        adjust_nt_mirroring(bus, addr);
        mem.write(bus, addr, bus.ppudata.reg);
        v = (v + (ppuctrl.addr_inc ? 32 : 1)) & 0x3fff;
        bus.ppudata.write_requested = false;
    }
    if (bus.ppudata.read_requested){
        unsigned short addr = v;
        adjust_nt_mirroring(bus, addr);
        bus.ppudata.reg = mem.read(bus, addr);
        v = (v + (ppuctrl.addr_inc ? 32 : 1)) & 0x3fff;
        bus.ppudata.read_requested = false;
    }
}

void PPU::adjust_nt_mirroring(Bus& bus, unsigned short& address) {
    for (int i = 0; i < 4; ++i){
        auto current_nt = constants::NAME_TABLE0_BEGIN + (i * 0x400);
        if (address >= current_nt  && address <= current_nt  + 0x3bf){
            auto r = get_nt_by_mirror_mode(bus.mapper.current_screen_mirroring, i);
            address = r + (address - current_nt);
            return;
        }
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

// https://www.nesdev.org/wiki/PPU_sprite_evaluation
void PPU::sprite_evaluate_tick(Bus &bus) {
    int sprite_sz = sprite_mode == m8x8 ? 8 : 16;
    // TODO: finish
    if (current_scanline_cycle <= 64){
    // Secondary OAM (32-byte buffer for current sprites on scanline) is initialized to $FF
    } else if (current_scanline_cycle >= 65 && current_scanline_cycle <= 256) {
        // todo: debug
        switch (current_sprite_evaluation_step) {
            case SpriteCopy: {
                //    On odd cycles, data is read from (primary) OAM
                //    On even cycles, data is written to secondary OAM
                 if (current_scanline_cycle & 1){
                     if (!copy_in_progress) {
                         current_oam_index = 4 * sprite_evaluation_index; // y_pos
                         tmp_oam_value = bus.oam[current_oam_index];
                         copy_in_progress = ((current_y >= tmp_oam_value  && current_y < tmp_oam_value + sprite_sz)
                                 && tmp_oam_value <= 239
                                 &&
                                 !(bus.oam[current_oam_index] == 0 &&
                                 bus.oam[current_oam_index+1] == 0 &&
                                 bus.oam[current_oam_index+2] == 0 &&
                                 bus.oam[current_oam_index+3] == 0)
                                 ); // + sprite height
                         if (!copy_in_progress) {
                             current_sprite_evaluation_step = SpriteIncrement;
                         } else {
                             tmp_oam_value += 0;
                         }
                     } else {
                         tmp_oam_value = bus.oam[current_oam_index + copy_step_count];
                     }
                 } else {
                     if (copy_in_progress) {
                         switch (copy_step_count) {
                             case 0:  {tmp_sprite.y_pos  =   tmp_oam_value;   break;}
                             case 1:  {tmp_sprite.pt_index = tmp_oam_value;   break;}
                             case 2:  {tmp_sprite.attrib =   tmp_oam_value;   break;}
                             case 3:  {tmp_sprite.x_pos =    tmp_oam_value;   break;}
                         }
                         ++copy_step_count;
                         if (copy_step_count == 4){
                             tmp_sprite.sprite_index = sprite_evaluation_index;
                             secondary_oam_buffer.push_back(tmp_sprite);
                             copy_step_count = 0;
                             copy_in_progress = false;
                             current_sprite_evaluation_step = SpriteIncrement;
                         }
                     }
                 }
                 break;
            }
            case SpriteIncrement: {
                ++sprite_evaluation_index;
                if (secondary_oam_buffer.size() == 8 && sprite_evaluation_index != 64) {
                    current_sprite_evaluation_step = SpriteOverflow;
                    ppustatus.sprite_overflow_flag = true; // todo: check
                    bus.ppustatus.reg |= 1 << 5;
                } else if (secondary_oam_buffer.size() <= 8 && sprite_evaluation_index == 64) {
                    current_sprite_evaluation_step = SpriteFinish;
                } else {
                    current_sprite_evaluation_step = SpriteCopy;
                }
                break;
            }
            case SpriteOverflow: {
                if (current_scanline_cycle & 1){
                    if (!copy_in_progress) {
                        current_oam_index =  4 * sprite_evaluation_index; // y_pos
                        copy_in_progress = (current_y >= tmp_oam_value  && current_y < tmp_oam_value + sprite_sz); // + sprite height
                    }
                } else {
                    if (copy_in_progress) {
                        ++copy_step_count;
                        if (copy_step_count == 4){
                            copy_step_count = 0;
                            copy_in_progress = false;
                        }
                    }
                    if (!copy_in_progress) {
                        ++sprite_evaluation_index;
                        if (sprite_evaluation_index == 64) {
                            current_sprite_evaluation_step = SpriteFinish;
                        }
                    }
                }
                break;
            }
            case SpriteFinish: break;
        }

    } else if (current_scanline_cycle >= 257 && current_scanline_cycle <= 320) {
        //  Sprite fetches
    }
}

void PPU::bg_tick(Bus &bus) {
    bus.irq_interrupt_flip_flop = bus.mapper.is_irq_requested();
    // TODO: continue debug
    update_state(bus);

    if (current_y < 240){
        if (!current_scanline_cycle){
#if DEBUG_MODE
            frame_start = false;
            line_start = false;
            if (current_y == 0 && current_scanline_cycle == 0 && (fill_white_background))
                internal_display.fill(WHITE);
#endif
            return;}

        // 1-256 cycles
        if (current_scanline_cycle >= 1 && current_scanline_cycle < 257 && ppumask.enable_background_rendering) {
            fetch_tile_data_step(bus);

#if DEBUG_MODE
            if (update_shifts) {
                tile_start = true;
                update_shifts = false;
            }
#endif

            if (current_scanline_cycle == 256){
                unsigned char y_fine_add_prev = ((v >> 12) & 0x7);
                unsigned char y_fine_add_val = ((((v >> 12) & 0x7) + 1) % 8);
                v &= 0x8fff;
                v |= y_fine_add_val << 12;

                if (y_fine_add_prev == 7){
                    auto prev = ((v >> 5) & 0x1f);
                    // todo: check prev == 31 case
                    auto add_val = ((((v >> 5) & 0x1f) + 1) % 30);
                    if (prev == 29)
                        v ^= 0x800;
                    v &= 0xfc1f;
                    v |= (add_val << 5);
                }
            }
        } else if(current_scanline_cycle == 257 && ppumask.enable_background_rendering) {
            // TODO: debug (oam_read test (v & 0x1f) == 2)
            // set coarse x + update ntx bit
            v &= 0xffe0;
            v |= (t & 0x1f);
            v &= 0xfbff;
            v |= (t & 0x400);
        } else if (current_scanline_cycle >= 321 && current_scanline_cycle <= 336 && ppumask.enable_background_rendering) {
            shift_register_high <<= 1;
            shift_register_low <<= 1;
            attributes_low <<= 1;
            attributes_high <<= 1;
            fetch_tile_data_step(bus);
        }
        if (current_scanline_cycle == 340){
#if DEBUG_MODE
            last_line_x =  std::exchange(current_line_x, 0);
            line_start = true;
#endif
        }
    } else if (current_y >= 240 && current_y < 261){
        if (current_y == 241 && current_scanline_cycle == 1) {
            ppustatus.vblank_flag = 1;
            bus.ppustatus.reg |= 1 << 7;
//            bus.current_nmi_cycle = 0;
//            bus.was_read_status = false;
            bus.nmi_interrupt_flip_flop = true;
        }
    } else if (current_y == 261){
        if (current_scanline_cycle == 1){
            std::copy(internal_display.begin(), internal_display.end(), display.begin());
            ppustatus.vblank_flag = 0;
            bus.ppustatus.reg &= 0x7f;
            bus.nmi_interrupt_flip_flop = false; // todo: check
//            bus.was_read_status = false;
        } else if (current_scanline_cycle == 280 && ppumask.enable_background_rendering) {
            v &= 0x841f;
            v |= ((t & (~0x841f & 0xffff)));
        } else if (current_scanline_cycle >= 321 && current_scanline_cycle <= 336 && ppumask.enable_background_rendering){
            shift_register_high <<= 1;
            shift_register_low <<= 1;
            attributes_low <<= 1;
            attributes_high <<= 1;
            fetch_tile_data_step(bus);
        } else if (current_scanline_cycle == 340 && ppumask.enable_background_rendering){
            ++frames_count;
#if DEBUG_MODE
                    frame_start = true;

#endif
        }
    }
}

void PPU::reset() {
    current_y = 261;
    current_x = 0;
    current_cycle = 0;
    v = 0;
    t = 0;
    x = 0;
    w = false;
    internal_display.fill(WHITE);
    display.fill(WHITE);
    secondary_oam_buffer.reset();
    secondary_oam.reset();
    current_scanline_cycle = 0;
    nt_byte = 0;
    attrib_value = 0;
    pattern_table_l = 0;
    pattern_table_h = 0;
    copy_in_progress = false;
    tmp_oam_value = 0;
    base_name_table_address = 0;
    bg_pattern_table_address = 0;
    base_attrib_address = 0;
    sprite_pattern_table_address8x8 = 0;
    shift_register_low = 0;
    shift_register_high = 0;
    attributes_low = 0;
    attributes_high = 0;
    current_step = NT;
    ppuctrl.reset();
    ppumask.reset();
    ppustatus.reset();
#if DEBUG_MODE
    run = false;
#endif
}

void PPU::update_base_nt(Bus& bus) {
    base_name_table_address = get_nt_by_mirror_mode(bus.mapper.current_screen_mirroring, ((v >> 10) & 0x3));
}

unsigned short PPU::get_nt_by_mirror_mode(Mapper::ScreenMirroring sm, unsigned char n) {
    using SM = Mapper::ScreenMirroring;
    switch (sm) {
        case SM::Vertical: { return (n == 0 || n == 2) ? constants::NAME_TABLE0_BEGIN : constants::NAME_TABLE1_BEGIN;}
        case SM::Horizontal: { return (n == 0 || n == 1) ? constants::NAME_TABLE0_BEGIN : constants::NAME_TABLE1_BEGIN;}
        case SM::SingleScreenLower: {return constants::NAME_TABLE0_BEGIN;}
        case SM::SingleScreenUpper: {return constants::NAME_TABLE1_BEGIN;}
        default:
            return constants::NAME_TABLE0_BEGIN;
    }
}

void PPU::fetch_tile_data_step(Bus& bus) {
    if (!(current_scanline_cycle & 1)) {
        unsigned char x_tile_n = (v & 0x1f);
        unsigned char y_tile_n = ((v >> 5) & 0x1f);
        update_base_nt(bus);
        switch (current_step) {
            case NT:  {
                nt_byte = mem.read(bus,  base_name_table_address + (y_tile_n * 32) + x_tile_n); current_step = AT;  break;} // 42
            case AT:  {
                auto val = mem.read(bus,   base_name_table_address + 0x3c0 + (y_tile_n/4 * 8) + x_tile_n/4);
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

                unsigned char prev = (v & 0x1f);
                unsigned char add_val = (((v & 0x1f) + 1) % 32);
                if (prev == 31)
                    v ^= 0x400;
                v &= 0xffe0;
                v |= add_val;

                attributes_low &= 0xff00;
                attributes_high &= 0xff00;

                attributes_low |= (attrib_value & 0x1) ? 0xff : 0;
                attributes_high |= (attrib_value & 0x2) ? 0xff : 0;
#if DEBUG_MODE
                update_shifts = true;
#endif
                current_step = NT;  break;}  // pattern table tile high
        }
    }
}

void PPU::draw_pixel(Bus &bus) {
    for (size_t i = 0, e = secondary_oam.size(); i != e; ++i){
        if (secondary_oam[i].x_count > 0) {
            --secondary_oam[i].x_count;
        } else {
            if ((secondary_oam[i].attrib >> 6) & 1){
                secondary_oam[i].shift_l >>= 1;
                secondary_oam[i].shift_h >>= 1;
            } else {
                secondary_oam[i].shift_l <<= 1;
                secondary_oam[i].shift_h <<= 1;
            }
        }
    }
    unsigned char sprite_data = 0;
    unsigned char sprite_attrib = 0;
    bool is_bg_prio = false;
    bool is_sprite = false;
    bool is_horizontal_flip = false;
    bool universal_background = false;
    unsigned char oam_index{};
    unsigned char cur_oam_index = secondary_oam.size() > 0 ? secondary_oam[0].oam_index : 0;
//    bool cur_above_background {};
    bool init_cur_above = false;
    for (size_t i = 0, e = secondary_oam.size(); i != e; ++i){
        if (secondary_oam[i].x_count == 0 && (secondary_oam[i].shift_l > 0 || secondary_oam[i].shift_h > 0)) {
                is_horizontal_flip = (secondary_oam[i].attrib >> 6) & 1;
                auto l_sprite_data =
                        is_horizontal_flip ?
                        ((secondary_oam[i].shift_h & 1) << 1)  | (secondary_oam[i].shift_l  & 1)
                                           : (((secondary_oam[i].shift_h >> 7) & 1) << 1)  | (secondary_oam[i].shift_l >> 7) & 1;
                if (!l_sprite_data) continue;
                if (cur_oam_index > (secondary_oam[i].oam_index)) continue;
                oam_index = secondary_oam[i].oam_index;
                sprite_data = l_sprite_data;

                sprite_attrib = secondary_oam[i].attrib & 0x3;
                is_bg_prio = (secondary_oam[i].attrib >> 5) & 1;
                cur_oam_index = oam_index;
                break;
        }
    }
    unsigned char bg_data = (((shift_register_high >> (15-x)) & 1) << 1) | (shift_register_low >> (15-x)) & 1;
    unsigned char res{};

    if (bg_data == 0 && (sprite_data != 0 && ppumask.enable_sprite_rendering)){
        is_sprite = true;
        res = sprite_data;
    } else if (bg_data != 0 && sprite_data == 0)
        res = bg_data;
    else if (bg_data != 0 && sprite_data != 0) {
        if (oam_index == 0){
            ppustatus.sprite_zero_hit_flag = true;
            bus.ppustatus.reg |= (1 << 6);
        }
        res = (!is_bg_prio && ppumask.enable_sprite_rendering) ? (is_sprite = true, sprite_data) : bg_data;
    } else {
        universal_background = true;
    }

    unsigned char attrib_high_bit = is_sprite ? ((sprite_attrib >> 1) & 1) :  (attributes_high >> (15-x)) & 1;
    unsigned char attrib_low_bit = is_sprite ? ((sprite_attrib) & 1) : (attributes_low >> (15-x)) & 1;

    unsigned char index = (((attrib_high_bit << 1) | attrib_low_bit) << 2) | res;

    
    unsigned char pixel_index = mem.read(bus, universal_background ? constants::IMAGE_PALETTE_BEGIN : (
                                         (is_sprite ? constants::SPRITE_PALETTE_BEGIN : constants::IMAGE_PALETTE_BEGIN) + index));

#if DEBUG_MODE
    if (bg_data == 0 && sprite_data != 0 && pixel_mixing_debug_view.get(PPU::PixelMixingDebugView::Mode::Bg))
        update_internal_display(ORANGE);
    else if  (bg_data != 0 && sprite_data == 0 && pixel_mixing_debug_view.get(PPU::PixelMixingDebugView::Mode::Sp))
        update_internal_display(PURPLE);
    else if  (bg_data != 0 && sprite_data != 0 && pixel_mixing_debug_view.get(PPU::PixelMixingDebugView::Mode::BgSp))
        update_internal_display(BROWN);
    else if (!bg_data  && !sprite_data && pixel_mixing_debug_view.get(PPU::PixelMixingDebugView::Mode::NBgSp))
        update_internal_display(GREEN);
    else
#endif
    update_internal_display(palette[pixel_index]);

    shift_register_high <<= 1;
    shift_register_low <<= 1;
    attributes_low <<= 1;
    attributes_high <<= 1;
}

void PPU::tick(Bus& bus) {
    ++current_cycle;
    bg_tick(bus);
    bus.mapper.tick();
    current_scanline_cycle += (current_scanline_cycle == 0 && current_y == 0 && ppumask.enable_background_rendering && (frames_count & 1));
#if DEBUG_MODE
    if (!is_sprite_off)
#endif
    sprite_evaluate_tick(bus);
    if (current_y < 240 && current_scanline_cycle >= 1 && current_scanline_cycle <= 256 && ppumask.enable_background_rendering) {
        draw_pixel(bus);
    } else if (current_scanline_cycle == 257) {
        // 257-320
        static unsigned short last_address = constants::PATTERN_TABLE1_BEGIN;
        secondary_oam.reset();
        if (ppumask.enable_background_rendering) {
            if (!secondary_oam_buffer.size()){
                // dummy read for IRQ
                mem.read(bus, last_address);
            }
            for (size_t i = 0, e = secondary_oam_buffer.size(); i != e; ++i) {
                bool is_vertical_flip = (secondary_oam_buffer[i].attrib >> 7) & 1;
                auto pt_base = (sprite_mode == m8x8 ?
                                (!ppuctrl.pattern_table_addr8x8 ? constants::PATTERN_TABLE0_BEGIN
                                                                : constants::PATTERN_TABLE1_BEGIN) :
                                (secondary_oam_buffer[i].pt_index & 1 ? constants::PATTERN_TABLE1_BEGIN
                                                                      : constants::PATTERN_TABLE0_BEGIN));
                auto pt_index = (sprite_mode == m8x8 ?
                        secondary_oam_buffer[i].pt_index :
                        secondary_oam_buffer[i].pt_index & 0xfe);
                auto diff =  (current_y - secondary_oam_buffer[i].y_pos);
                unsigned char current_index = pt_index;
                if (diff < 8){
                    current_index += (sprite_mode == PPU::SpriteMode::m8x16 && is_vertical_flip);
                } else {
                    current_index += (!is_vertical_flip);
                }
                auto addr =  pt_base + (current_index * 16) + (is_vertical_flip ? (7 - (diff % 8)) : (diff % 8));
                auto l = mem.read(bus, addr);
                auto h = mem.read(bus, addr + 8);
                last_address = addr;

                secondary_oam.push_back(SpriteData{
                        .shift_l =  l,
                        .shift_h =  h,
                        .x_count =  secondary_oam_buffer[i].x_pos,
                        .attrib  =  secondary_oam_buffer[i].attrib,
                        .oam_index =  secondary_oam_buffer[i].sprite_index,
                });
            }
        }
        secondary_oam_buffer.reset();
    }
    ++current_scanline_cycle;
    if (current_scanline_cycle == 341){
        current_sprite_evaluation_step = SpriteCopy;
        current_scanline_cycle = 0;
        sprite_evaluation_index = 0;
        if (current_y == 261){
            ppustatus.sprite_zero_hit_flag = false;
            bus.ppustatus.reg &= 0xbf;
            ppustatus.sprite_overflow_flag = false;
            bus.ppustatus.reg &= ((~(1 << 5)) & 0xff);
        }
        current_y = current_y == 261 ? (current_x = 0) : current_y + 1;

    }
}