#include "Memory.h"

bool Memory::lcd_enabled() const {
    return map[constants::LCDC_ADDRESS] & 0x80;
}

void Memory::handle_mbc(unsigned short index, unsigned char value) {
    switch (current_mbc) {
        case MBC0: {handle_mbc0(index, value); break;}
        case MBC1: {handle_mbc1(index, value); break;}
        case MBC3: {handle_mbc3(index, value); break;}
    }
}

void Memory::handle_mbc0(unsigned short index, unsigned char value) {
    if (index >= 0x0000 && index <= 0x1FFF) external_ram_enabled = (value & 0xf) == 0x0A;
}


void Memory::handle_mbc1(unsigned short index, unsigned char value) {
    // MBC-1
    // https://hacktix.github.io/GBEDG/mbcs/mbc1/
    if (index <= 0x1FFF) {
        external_ram_enabled = (value & 0xf) == 0x0A;
    } else if (index >= 0x2000 && index <= constants::ROM_BANK_00_END) {
        auto v = value & 0x1f;
        if (v == 0) v = 1;
        mbc_low_bits = v;
    } else if (index >= constants::ROM_BANK_01_BEGIN && index <= 0x5FFF) {
        mbc_high_bits = value & 0x3;
    }  else if (index >= 0x6000 && index <= constants::ROM_BANK_01_END) {
        if (value & 1) current_banking_mode = Advanced;
        else current_banking_mode = Simple;
    }

    if (current_banking_mode == Simple) {
        current_cart_ram = 0;
        if (rom_sz <= 32) {
            current_rom_bank0 = 0;
            current_rom_bank1 = mbc_low_bits % rom_sz;
        } else if (rom_sz == 64){
            current_rom_bank0 = 0;
            current_rom_bank1 = (((mbc_high_bits & 1) << 5) | mbc_low_bits) % rom_sz;
        } else if (rom_sz >= 128) {
            current_rom_bank0 = 0;
            current_rom_bank1 = ((mbc_high_bits << 5) | mbc_low_bits) % rom_sz;
        }
    } else {
        if (mbc_high_bits <= max_ram_bank_value)
            current_cart_ram = mbc_high_bits;
        if (rom_sz <= 32) {
            current_rom_bank0 = 0;
            current_rom_bank1 =  mbc_low_bits % rom_sz;
        } else if (rom_sz == 64){
            current_rom_bank0 =  ((mbc_high_bits & 1) << 5) % rom_sz;
            current_rom_bank1 =  ((mbc_high_bits & 1) << 5 | mbc_low_bits) % rom_sz;
        } else if (rom_sz >= 128) {
            current_rom_bank0 = (mbc_high_bits << 5) % rom_sz;
            current_rom_bank1 = ((mbc_high_bits << 5) | mbc_low_bits) % rom_sz;
        }
    }
}

void Memory::handle_mbc3(unsigned short index, unsigned char value) {
    if (index <= 0x1FFF) {
        external_ram_enabled = (value & 0xf) == 0x0A;
    } else if (index >= 0x2000 && index <= constants::ROM_BANK_00_END) {
        if (value == 0) value = 1;
        mbc_low_bits = value;
    } else if (index >= constants::ROM_BANK_01_BEGIN && index <= 0x5FFF) {
        mbc_high_bits = value & 0x3;
    }  else if (index >= 0x6000 && index <= constants::ROM_BANK_01_END) {
        if (value & 1) current_banking_mode = Advanced;
        else current_banking_mode = Simple;
    }

    if (current_banking_mode == Simple) {
        current_cart_ram = 0;
        if (rom_sz <= 32) {
            current_rom_bank0 = 0;
            current_rom_bank1 = mbc_low_bits % rom_sz;
        } else if (rom_sz == 64){
            current_rom_bank0 = 0;
            current_rom_bank1 = (((mbc_high_bits & 1) << 5) | mbc_low_bits) % rom_sz;
        } else if (rom_sz >= 128) {
            current_rom_bank0 = 0;
            current_rom_bank1 = ((mbc_high_bits << 5) | mbc_low_bits) % rom_sz;
        }
    } else {
        if (mbc_high_bits <= max_ram_bank_value)
            current_cart_ram = mbc_high_bits;
        if (rom_sz <= 32) {
            current_rom_bank0 = 0;
            current_rom_bank1 =  mbc_low_bits % rom_sz;
        } else if (rom_sz == 64){
            current_rom_bank0 =  ((mbc_high_bits & 1) << 5) % rom_sz;
            current_rom_bank1 =  ((mbc_high_bits & 1) << 5 | mbc_low_bits) % rom_sz;
        } else if (rom_sz >= 128) {
            current_rom_bank0 = (mbc_high_bits << 5) % rom_sz;
            current_rom_bank1 = ((mbc_high_bits << 5) | mbc_low_bits) % rom_sz;
        }
    }
}

bool Memory::is_ram_avaliable(unsigned short index) const {
    auto current_ppu_mode = get_ppu_mode();
    return !(((index >=  constants::VRAM_BEGIN && index <=  constants::VRAM_END) && ((current_ppu_mode == 3 && lcd_enabled()))) ||
             ((index >=  constants::EXTERNAL_RAM_BEGIN && index <=  constants::EXTERNAL_RAM_END) && (!external_ram_enabled)) || // todo: check external Memory
             ((index >=  constants::OAM_BEGIN && index <= constants::OAM_END) && ppu_oam_usage) || // (((current_ppu_mode == 2 || current_ppu_mode == 3) && lcd_enabled())))
             ((index >=  constants::NOT_USABLE_BEGIN && index <=  constants::NOT_USABLE_END))
    );
//             ||
//             (!(index >= constants::HRAM_BEGIN && index <= constants::HRAM_END) && dma_active));
}

unsigned char Memory::get_ppu_mode() const {
    return map[constants::STAT_ADDRESS] & 0x03;
}

void Memory::write(unsigned short index, unsigned char value) {
    if (index >= constants::OAM_BEGIN && index <= constants::OAM_END && dma_active){
        return;
    }

    if (index <= constants::ROM_BANK_01_END){
        handle_mbc(index, value);
        return;
    }

    if (index == constants::DMA) {
        dma_active = true;
        dma_address = (value << 8 | 0x00);
        dma_counter = 0;
        dma_t = 0;
    }

    if (index >= constants::ECHO_RAM_BEGIN && index <= constants::ECHO_RAM_END){
        index -= 0x2000;
    }

   if (index >= constants::EXTERNAL_RAM_BEGIN && index <= constants::EXTERNAL_RAM_END && external_ram_enabled && !cart_ram.empty()) {
        auto current_address = index - constants::EXTERNAL_RAM_BEGIN;
        if (current_address + (current_cart_ram * 8192) <= cart_ram.size())
            cart_ram[current_address + (current_cart_ram * 8192)] = value;
       return;
   }

    if (index == constants::DIV_REGISTER){
        value = 0;
    }

    if (index == constants::TIMA_REGISTER) {
        tima_written = true;
    }

    if (index == constants::DIV_REGISTER){
        div_written = true;
    }

    if (is_ram_avaliable(index)){
        map[index] = value;
    }
}

unsigned char Memory::read(unsigned short index) const {
    if (index >= constants::OAM_BEGIN && index <= constants::OAM_END && dma_active){
        return dummy_reg;
    }

    if (index >= constants::ROM_BANK_00_BEGIN && index <= constants::ROM_BANK_00_END){
        return rom[(current_rom_bank0 * constants::BANK_ROM_SZ) + index];
    }

    if (index >= constants::ROM_BANK_01_BEGIN && index <= constants::ROM_BANK_01_END) {
        return rom[(current_rom_bank1 * constants::BANK_ROM_SZ) + (index - constants::ROM_BANK_01_BEGIN)];
    }

    if (index >= constants::ECHO_RAM_BEGIN && index <= constants::ECHO_RAM_END){
        std::cout << "ECHO Memory READ!" << '\n';
        index -= 0x2000;
    }

    if (index == constants::DIV_REGISTER){
        return div_internal_sync >> 8;
    }

    if (index >= constants::EXTERNAL_RAM_BEGIN && index <= constants::EXTERNAL_RAM_END) {
        return external_ram_enabled ? cart_ram[(index - constants::EXTERNAL_RAM_BEGIN) + (current_cart_ram * 8192)] : dummy_reg;
    }

    if (is_ram_avaliable(index)){
        return map[index];
    }

    return dummy_reg;
}

void Memory::write_unsafe(unsigned short index, unsigned char value) {
    if (index <= constants::ROM_BANK_01_END){
        handle_mbc(index, value);
        return;
    }

    if (index == constants::DMA) {
        dma_active = true;
        dma_address = (value << 8 | 0x00);
        dma_counter = 0;
        dma_t = 0;
    }

    if (index == constants::DIV_REGISTER){
        value = 0;
    }

    if (index >= constants::ECHO_RAM_BEGIN && index <= constants::ECHO_RAM_END){
        index -= 0x2000;
    }

    if (index == constants::TIMA_REGISTER) {
        tima_written = true;
        if (tima_overflow || glitch_overflow) {
            tima_overflow = false;
            glitch_overflow = false;
        }
    }

    if (index == constants::DIV_REGISTER){
        div_written = true;
    }

    if (index == constants::TAC_REGISTER){
        bool old_enabled = (map[constants::TAC_REGISTER] >> 2) & 1;
        auto old_value = map[constants::TAC_REGISTER];
        map[constants::TAC_REGISTER] = value;

        unsigned char shift = 0;
        switch (old_value & 3) {
            case 0b00: {shift = 9; break;}
            case 0b01: {shift = 3; break;}
            case 0b10: {shift = 5; break;}
            default:
                shift = 7;
                break;
        }
        if (old_enabled && (div_internal_sync >> shift) & 1){
            unsigned char prev = read(constants::TIMA_REGISTER);
            map[constants::TIMA_REGISTER] = prev + 1;
            if (prev == 0xff) {
                map[constants::IF_ADDRESS] = read(constants::IF_ADDRESS) | (1 << 2);
                glitch_overflow = true;
            }
        }
        return;
    }

    if (index >= constants::EXTERNAL_RAM_BEGIN && index <= constants::EXTERNAL_RAM_END) {
        auto current_address = index - constants::EXTERNAL_RAM_BEGIN;
        cart_ram[current_address + (current_cart_ram * 8192)] = value;
        return;
    }

    map[index] = value;
}

unsigned char Memory::read_unsafe(unsigned short index) const {
    if (index >= constants::ROM_BANK_00_BEGIN && index <= constants::ROM_BANK_00_END){
        return rom[(current_rom_bank0 * constants::BANK_ROM_SZ) + index];
    }

    if (index >= constants::ROM_BANK_01_BEGIN && index <= constants::ROM_BANK_01_END) {
        return rom[(current_rom_bank1 * constants::BANK_ROM_SZ) + (index - constants::ROM_BANK_01_BEGIN)];
    }

    if (index >= constants::ECHO_RAM_BEGIN && index <= constants::ECHO_RAM_END){
        index -= 0x2000;
    }

    if (index == constants::DIV_REGISTER){
        return div_internal_sync >> 8;
    }

    if (index >= constants::EXTERNAL_RAM_BEGIN && index <= constants::EXTERNAL_RAM_END) {
        if (external_ram_enabled)
            return cart_ram[(index - constants::EXTERNAL_RAM_BEGIN) + (current_cart_ram * 8192)];
        return dummy_reg;
    }
    return map[index];
}

void Memory::dma_tick() {
    ++dma_t;
    if (dma_t % 4 == 0) {
        write_unsafe(constants::OAM_BEGIN + dma_counter, read_unsafe(dma_address + dma_counter));
        ++dma_counter;
        if (dma_counter == 160) {
            dma_active = false;
            dma_address = 0;
            dma_counter = 0;
        }
    }
}
