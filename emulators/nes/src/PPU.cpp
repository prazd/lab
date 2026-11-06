#include "PPU.hpp"
#include "Constants.hpp"
#include <bitset>
#include <cassert>

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
    bool bit_plane = addr & 0x8;
    unsigned char tile_number = (addr >> 4) & 0xff;
    unsigned short base = (addr >> 12) & 1 ? constants::PATTERN_TABLE1_BEGIN : constants::PATTERN_TABLE0_BEGIN;

    return bus.read_chr( base + (tile_number * 16) + y_offset + (bit_plane ? 8 : 0));
}

unsigned short PPU::to_pattern_addr(unsigned char y, unsigned char tile_n, bool plane, bool h) {
    return y | (plane << 3) | (tile_n << 4) | (h << 12);
}

// https://www.nesdev.org/wiki/PPU_sprite_evaluation
void PPU::sprite_evaluate_tick(Bus& bus) {
    int sprite_sz = bus.sprite_mode == Bus::SpriteMode::m8x8 ? 8 : 16;
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
                         tmp_oam_value = bus.oam[bus.oamaddr];
                         copy_in_progress = ((current_y >= tmp_oam_value  && current_y < tmp_oam_value + sprite_sz)
                                 && tmp_oam_value <= 239
                                 &&
                                 !(bus.oam[bus.oamaddr] == 0 &&
                                 bus.oam[bus.oamaddr+1] == 0 &&
                                 bus.oam[bus.oamaddr+2] == 0 &&
                                 bus.oam[bus.oamaddr+3] == 0)
                                 ); // + sprite height
                         if (!copy_in_progress) {
                             current_sprite_evaluation_step = SpriteIncrement;
                         } else if (secondary_oam_buffer.size() == 8){
                             bus.ppustatus |= 1 << 5;
                             current_sprite_evaluation_step = SpriteOverflow;
                             copy_in_progress = false;
                         }
                     } else {
                         tmp_oam_value = bus.oam[bus.oamaddr + copy_step_count];
                     }
                 } else {
                     if (current_scanline_cycle == 66) tmp_sprite.is_zero_sprite = true;
                     if (copy_in_progress) {
                         switch (copy_step_count) {
                             case 0:  {tmp_sprite.y_pos  =   tmp_oam_value;   break;}
                             case 1:  {tmp_sprite.pt_index = tmp_oam_value;   break;}
                             case 2:  {tmp_sprite.attrib =   tmp_oam_value;   break;}
                             case 3:  {tmp_sprite.x_pos =    tmp_oam_value;   break;}
                         }
                         ++copy_step_count;
                         if (copy_step_count == 4){
                             secondary_oam_buffer.push_back(tmp_sprite);
                             copy_step_count = 0;
                             copy_in_progress = false;
                             current_sprite_evaluation_step = SpriteIncrement;
                             tmp_sprite.is_zero_sprite = false;
                         }
                     }
                 }
                 break;
            }
            case SpriteIncrement: {
                bus.ppu_oamfetch_count += 4;
                bus.oamaddr += 4;
                if (secondary_oam_buffer.size() <= 8 && bus.ppu_oamfetch_count == 256) {
                    current_sprite_evaluation_step = SpriteFinish;
                } else {
                    current_sprite_evaluation_step = SpriteCopy;
                }
                break;
            }
            case SpriteOverflow: {
                if (current_scanline_cycle & 1){
// todo: check
//                    if (!copy_in_progress) {
//                        current_oam_index =  4 * sprite_evaluation_index; // y_pos
//                        copy_in_progress = (current_y >= tmp_oam_value  && current_y < tmp_oam_value + sprite_sz); // + sprite height
//                    }
                } else {
                    if (copy_in_progress) {
                        ++copy_step_count;
                        if (copy_step_count == 4){
                            copy_step_count = 0;
                            copy_in_progress = false;
                        }
                    }
                    if (!copy_in_progress) {
                        bus.oamaddr += 4;
                        bus.ppu_oamfetch_count += 4;
                        if (bus.ppu_oamfetch_count == 256) {
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
    auto& v = bus.ppu_v;
    // TODO: continue debug
//    update_state(bus);
#if DEBUG_MODE
    if (current_scanline_cycle == 0 && current_y == 0){
        frame_start = false;
        if (fill_white_background) get_active_display().fill(WHITE);
    } else if (current_scanline_cycle == 340){
        last_line_x =  std::exchange(current_line_x, 0);
        line_start = true;
    }
#endif
    if (current_y < 240){
        // 1-256 cycles
        if (current_scanline_cycle >= 1 && current_scanline_cycle <= 256) {
            fetch_tile_data_step(bus);

#if DEBUG_MODE
            if (update_shifts) {
                tile_start = true;
                update_shifts = false;
            }
#endif
            if (current_scanline_cycle == 256 && bus.ppumask & 0x18)
                bus.inc_ppu_v_y();
        } else if(current_scanline_cycle == 257 && bus.ppumask & 0x18) {
            // TODO: debug (oam_read test (v & 0x1f) == 2)
            // set coarse x + update ntx bit
            v &= 0xfbe0;
            v |= (bus.ppu_t & 0x41f);
#if DEBUG_MODE
            // TODO: debug
            if (current_y == 0)
                bus.start_v_value = v;
#endif
            bus.mapper.irq_handle();
        } else if (current_scanline_cycle >= 321 && current_scanline_cycle <= 336) {
            shift_register_high <<= 1;
            shift_register_low <<= 1;
            attributes_low <<= 1;
            attributes_high <<= 1;
            fetch_tile_data_step(bus);
        }
    } else if (current_y == 261){
        if (current_scanline_cycle >= 1 && current_scanline_cycle < 257) {
            // todo: check
            shift_register_high <<= 1;
            shift_register_low <<= 1;
            attributes_low <<= 1;
            attributes_high <<= 1;
            fetch_tile_data_step(bus);
        } else if (current_scanline_cycle == 280 && bus.ppumask & 0x18) {
            v &= 0x841f;
            v |= (bus.ppu_t & 0x7be0);
#if DEBUG_MODE
            bus.start_v_value = v;
#endif
            bus.mapper.irq_handle();
        } else if (current_scanline_cycle >= 321 && current_scanline_cycle <= 336){
            shift_register_high <<= 1;
            shift_register_low <<= 1;
            attributes_low <<= 1;
            attributes_high <<= 1;
            fetch_tile_data_step(bus);
        } else if (current_scanline_cycle == 340){ // todo: check ppumask.enable_background_rendering
#if DEBUG_MODE
            frame_start = true;
#endif
        }
    }
}

void PPU::reset() {
    current_y = 0;
    current_x = 0;
    current_cycle = 0;
    displays[0].fill(BLACK);
    displays[1].fill(BLACK);
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
    shift_register_low = 0;
    shift_register_high = 0;
    attributes_low = 0;
    attributes_high = 0;
    current_step = NT;
#if DEBUG_MODE
    run = false;
    frame_start = false;
#endif
}

void PPU::update_base_nt(Bus& bus) {
    base_name_table_address = bus.get_nt_by_mirror_mode(bus.mapper.current_screen_mirroring, ((bus.ppu_v >> 10) & 0x3));
}

void PPU::tile_data_fetch(Bus& bus) {
    auto& v = bus.ppu_v;
    unsigned char x_tile_n = (v & 0x1f);
    unsigned char y_tile_n = ((v >> 5) & 0x1f);
    update_base_nt(bus);
    switch (current_step) {
        case NT:  {
            nt_byte = bus.read_chr(  base_name_table_address + (y_tile_n * 32) + x_tile_n);
            current_step = AT;
            break;} // 42
        case AT:  {
            auto val = bus.read_chr(   base_name_table_address + 0x3c0 + (y_tile_n/4 * 8) + x_tile_n/4);
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
            pattern_table_l = tile_byte_from_addr(bus, to_pattern_addr(((v >> 12) & 0x7), nt_byte, false, bus.ppuctrl & 0x10));
            current_step = PTH; break;}     // pattern table tile low
        case PTH: {
            pattern_table_h = tile_byte_from_addr(bus, to_pattern_addr(((v >> 12) & 0x7), nt_byte, true, bus.ppuctrl & 0x10));
            shift_register_low &= 0xff00;
            shift_register_low |=  pattern_table_l;
            shift_register_high &= 0xff00;
            shift_register_high |= pattern_table_h;

            bus.inc_ppu_v_x();

            attributes_low &= 0xff00;
            attributes_high &= 0xff00;

            attributes_low |= (attrib_value & 0x1) ? 0xff : 0;
            attributes_high |= (attrib_value & 0x2) ? 0xff : 0;
#if DEBUG_MODE
            update_shifts = true;
#endif
            current_step = NT;
            break;}  // pattern table tile high
    }
}

void PPU::tile_data_dummy_fetch(Bus& bus) {
    switch (current_step) {
        case NT: {
            current_step = AT;
            break;}
        case AT:  {
            current_step = PTL;
            break;}
        case PTL: {
            current_step = PTH;
            break;}
        case PTH: {
            current_step = NT;
            break;}
    }
}

void PPU::fetch_tile_data_step(Bus& bus) {
    if (!(current_scanline_cycle & 1)) {
        if (bus.ppumask & 0x18)
            tile_data_fetch(bus);
        else
            tile_data_dummy_fetch(bus);
    }
}

void PPU::draw_pixel(Bus &bus) {
    unsigned char sprite_data = 0;
    unsigned char sprite_attrib = 0;
    bool is_bg_prio = false;
    bool is_sprite = false;
    bool is_horizontal_flip = false;
    bool universal_background = false;
    unsigned char oam_index{};
//    unsigned char cur_oam_index = secondary_oam.size() > 0 ? secondary_oam[0].oam_index : 0;
    size_t scanline_x = current_x % 256;
    bool is_zero_sprite = false;
    for (size_t i = 0, e = secondary_oam.size(); i != e; ++i){
        if (secondary_oam[i].x_pos <= scanline_x && (secondary_oam[i].shift_l > 0 || secondary_oam[i].shift_h > 0)) {
                is_horizontal_flip = secondary_oam[i].attrib & 0x40;
                auto l_sprite_data =
                        is_horizontal_flip ?
                        ((secondary_oam[i].shift_h & 1) << 1)  | (secondary_oam[i].shift_l  & 1)
                                           : (((secondary_oam[i].shift_h >> 7) & 1) << 1) | (secondary_oam[i].shift_l >> 7) & 1;
                if (!l_sprite_data) continue;
//                if (cur_oam_index > (secondary_oam[i].oam_index)) continue;
//                oam_index = secondary_oam[i].oam_index;
                sprite_data = l_sprite_data;
                is_zero_sprite = secondary_oam[i].is_zero_sprite;

                sprite_attrib = secondary_oam[i].attrib & 0x3;
                is_bg_prio = (secondary_oam[i].attrib >> 5) & 1;
//                cur_oam_index = oam_index;
                break;
        }
    }
    unsigned char bg_data = (((shift_register_high >> (15-bus.ppu_x)) & 1) << 1) | (shift_register_low >> (15 - bus.ppu_x)) & 1;
    unsigned char res{};

    // clipping
    if (current_x % 256 <= 7) { bg_data *= (bus.ppumask >> 1) & 1; sprite_data *= (bus.ppumask >> 2) & 1;}

    if (bg_data == 0 && (sprite_data != 0 && bus.ppumask & 0x10)){
        is_sprite = true;
        res = sprite_data;
    } else if (bg_data != 0 && sprite_data == 0){
        res = (universal_background = !(bus.ppumask & 0x8), bg_data); // TODO: check
    } else if (bg_data != 0 && sprite_data != 0) { // zero hit
        // TODO: debug
        if (is_zero_sprite && bus.ppumask & 0x10 && bus.ppumask & 0x8 && ((current_x & 0xff) != 0xff) && !(bus.ppustatus & (0x80)) && !(bus.ppustatus & 0x40)){
            bus.ppustatus |= (1 << 6);
        }
        res = (!is_bg_prio && (bus.ppumask & 0x10)) ? (is_sprite = true, sprite_data) : (universal_background = !(bus.ppumask & 0x8), bg_data); // TODO: check
    } else {
        universal_background = true;
    }

#if DEBUG_MODE
    if (pixel_mixing_debug_view.get(PPU::PixelMixingDebugView::SpriteOnly)){
        res = sprite_data;
        is_sprite = true;
    } else if (pixel_mixing_debug_view.get(PPU::PixelMixingDebugView::BgOnly)){
        res = bg_data;
        is_sprite = false;
    }
#endif

    bool attrib_high_bit = is_sprite ? (sprite_attrib & 0x2) : (attributes_high >> (15-bus.ppu_x)) & 1;
    bool attrib_low_bit = is_sprite ? ((sprite_attrib) & 1) : (attributes_low >> (15-bus.ppu_x)) & 1;

    unsigned char index = (((attrib_high_bit << 1) | attrib_low_bit) << 2) | res;


    unsigned char pixel_index = 0;
    if (bus.ppumask & 0x18) {
        pixel_index = bus.read_chr( universal_background ? constants::IMAGE_PALETTE_BEGIN : (
                (is_sprite ? constants::SPRITE_PALETTE_BEGIN : constants::IMAGE_PALETTE_BEGIN) + index));
    } else if (bus.ppu_v >= constants::IMAGE_PALETTE_BEGIN && bus.ppu_v <= constants::SPRITE_PALETTE_END){
        pixel_index = bus.read_chr(bus.ppu_v) & 0x3f;
    } else {
        pixel_index = bus.read_chr(constants::IMAGE_PALETTE_BEGIN); // TODO: check
    }
    if (bus.ppumask & 1) pixel_index &= 0x30;
#if !HIDE_OVERSCAN
    if (current_y <= 7 || current_y >= 232 || current_x % 256 <= 15 || current_x % 256 >= 240)
        pixel_index = 13; // BLACK
#endif
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

#if DEBUG_MODE
    if (show_current_pixel && current_x < std::size(displays[0]))
        displays[current_display_index][current_x] = RED;
#endif

    shift_register_high <<= 1;
    shift_register_low <<= 1;
    attributes_low <<= 1;
    attributes_high <<= 1;
    for (size_t i = 0, e = secondary_oam.size(); i != e; ++i){
        if (secondary_oam[i].x_pos <= scanline_x && (secondary_oam[i].shift_l > 0 || secondary_oam[i].shift_h > 0)) {
            if (secondary_oam[i].attrib & 0x40){
                secondary_oam[i].shift_l >>= 1;
                secondary_oam[i].shift_h >>= 1;
            } else {
                secondary_oam[i].shift_l <<= 1;
                secondary_oam[i].shift_h <<= 1;
            }
        }
    }
}

void PPU::secondary_oam_fill(Bus& bus){
    secondary_oam.reset();
    if (bus.ppumask & 0x18) {
        if (secondary_oam_buffer.empty()){
            // dummy read for IRQ
            bus.read_chr( last_address);
        }
        for (size_t i = 0, e = secondary_oam_buffer.size(); i != e; ++i) {
            bool is_vertical_flip = secondary_oam_buffer[i].attrib & 0x80;
            auto pt_base = (bus.sprite_mode == Bus::SpriteMode::m8x8 ?
                            (!(bus.ppuctrl & 0x8) ? constants::PATTERN_TABLE0_BEGIN
                                                       : constants::PATTERN_TABLE1_BEGIN) :
                            (secondary_oam_buffer[i].pt_index & 1 ? constants::PATTERN_TABLE1_BEGIN
                                                                  : constants::PATTERN_TABLE0_BEGIN));
            auto pt_index = (bus.sprite_mode == Bus::SpriteMode::m8x8 ?
                             secondary_oam_buffer[i].pt_index :
                             secondary_oam_buffer[i].pt_index & 0xfe);
            auto diff =  (current_y - secondary_oam_buffer[i].y_pos);
            unsigned char current_index = pt_index;
            if (diff < 8){
                current_index += (bus.sprite_mode == Bus::SpriteMode::m8x16 && is_vertical_flip);
            } else {
                current_index += (!is_vertical_flip);
            }
            auto addr =  pt_base + (current_index * 16) + (is_vertical_flip ? (7 - (diff % 8)) : (diff % 8));
            auto l = bus.read_chr(addr);
            auto h = bus.read_chr( addr + 8);
            last_address = addr;

            secondary_oam.push_back(SpriteData{
                    .shift_l =  l,
                    .shift_h =  h,
                    .x_pos =  secondary_oam_buffer[i].x_pos,
                    .attrib  =  secondary_oam_buffer[i].attrib,
                    .is_zero_sprite =  secondary_oam_buffer[i].is_zero_sprite,
            });
        }
        bus.oamaddr = 0;
        bus.ppu_oamfetch_count = 0;
    }
    secondary_oam_buffer.reset();
}

void PPU::tick(Bus& bus) {
    if (bus.ppu_v_update_delay){
        --bus.ppu_v_update_delay;
        if (!bus.ppu_v_update_delay) {
            bus.mapper.update_state(bus.ppu_t, bus.ppu_v);
            bus.ppu_v = bus.ppu_t;
#if DEBUG_MODE
            bus.start_v_value = bus.ppu_v;
#endif
        }
    }
    if (bus.ppu_mask_update_delay){
        --bus.ppu_mask_update_delay;
        if (!bus.ppu_mask_update_delay)
            bus.ppumask = bus.ppu_mask_next_value;
    }

    if (current_y < 240 && current_scanline_cycle >= 1 && current_scanline_cycle <= 256) {
        draw_pixel(bus);
        if (bus.ppumask & 0x18) sprite_evaluate_tick(bus);
    }
    if (current_y < 240 && current_scanline_cycle == 257)
        secondary_oam_fill(bus);

    bg_tick(bus);
    bus.mapper.tick();
#if DEBUG_MODE
    if (!is_sprite_off)
#endif

    ++current_scanline_cycle;

    if (current_y == 241 && current_scanline_cycle == 1 && (current_cycle - bus.status_read_cycle > 1)) {
        bus.ppustatus &= 0x5f; // todo: check zero hit value set to zero
        bus.ppustatus |= 1 << 7;
        bus.current_nmi_cycle = current_cycle;
        if (bus.ppuctrl & 0x80) bus.nmi_interrupt_flip_flop = true;
    }

    if (((current_y == 241 && current_scanline_cycle > 1) || current_y >= 242) && current_y < 261
     && bus.ppuctrl & 0x80 && !(prev_ppuctrl_val & 0x80) && bus.ppustatus & 0x80) {
        bus.current_nmi_cycle = current_cycle;
        bus.nmi_interrupt_flip_flop = true;
        bus.is_ppu_nmi_latch_active = true;
    }

    if (current_y == 261){
        if (bus.ppumask & 0x18) sprite_evaluate_tick(bus);
        if (current_scanline_cycle == 1) {
            update_current_display();
            bus.nmi_interrupt_flip_flop = false;
            bus.ppustatus &= 0x7f;
        } else if (current_scanline_cycle == 257) {
            secondary_oam_fill(bus);
        } else if (current_scanline_cycle == 340 && bus.ppumask & 0x8 && (frames_count & 1)) {
            ++current_scanline_cycle;
        }
    }
    bus.current_ppu_cycle = current_cycle;
    if (current_scanline_cycle == 341){
#if DEBUG_MODE
       if (current_y == 261) frame_start = true;
#endif
        current_sprite_evaluation_step = SpriteCopy;
        current_scanline_cycle = 0;
        current_y = current_y == 261 ? (++frames_count, current_x = 0) : current_y + 1;
#if DEBUG_MODE
        assert(current_step == NT);
#endif
        if (current_y == 261){
            bus.ppustatus &= 0x8f;
        }
    }
    bus.ppu_current_y = current_y;
    prev_ppuctrl_val = bus.ppuctrl;
}