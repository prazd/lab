#include "Memory.h"
#include <ctime>
#if CHEATS_ON
#include "Cheats.h"
#endif

bool Memory::lcd_enabled() const {
    return map[constants::LCDC_ADDRESS] & 0x80;
}

void Memory::handle_mbc(unsigned short index, unsigned char value) {
    switch (current_mbc) {
        case MBC0: {handle_mbc0(index, value); break;}
        case MBC1: {handle_mbc1(index, value); break;}
        case MBC2: {handle_mbc2(index, value); break;}
        case MBC3: {handle_mbc3(index, value); break;}
        case MBC5: {handle_mbc5(index, value); break;}
    }
}

void Memory::handle_mbc0(unsigned short index, unsigned char value) {
    if (index >= 0x0000 && index <= 0x1FFF) external_ram_enabled = (value & 0xf) == 0x0A;
}

void Memory::handle_mbc2(unsigned short index, unsigned char value) {
    if (index <= constants::ROM_BANK_00_END) {
        if ((index >> 8) & 1){
            auto v = value & 0xf;
            if (v == 0) v = 1;
            current_rom_bank1 = v % 16;
        } else {
            external_ram_enabled = ((value & 0xf) == 0xA);
        }
    }
}

void Memory::handle_mbc5(unsigned short index, unsigned char value) {
    if (index <= 0x1FFF) {
        if (value == 0xA) external_ram_enabled = true;
        else if (value == 0) external_ram_enabled = false;
    } else if (index >= 0x2000 && index <= 0x2fff) {
        mbc_low_bits = value;
        current_rom_bank1 = ((mbc_high_bits << 8) | mbc_low_bits);
    } else if (index >= 0x3000 && index <=  0x3fff) {
        mbc_high_bits = (value & 0x1);
        current_rom_bank1 = ((mbc_high_bits << 8) | mbc_low_bits);
    } else if (index >= 0x4000 && index <= 0x5FFF) {
        if (value <= max_ram_bank_value) current_external_ram_n = value;
    }
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
        current_external_ram_n = 0;
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
            current_external_ram_n = mbc_high_bits;
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
    static bool prev_00 = false;
    if (index >= 0x2000 && index <= 0x3fff){
        current_rom_bank1 = value;
    }
    if (index >= 0x4000 && index <= 0x5fff){
        current_external_ram_n = value;
    }
    if (index >= 0x6000 && index <= 0x7fff){
        if (value == 0) {prev_00 = true; return;}
        if (value == 1){
            prev_00 = false;
            if ((map[0xC] >> 6) & 0x1) return; // HALT
            auto now = std::chrono::system_clock::now();
            auto t = std::chrono::system_clock::to_time_t(now);
            auto local = *std::localtime(&t);
            map[0x08] = local.tm_sec;
            map[0x09] = local.tm_min;
            map[0xA] = local.tm_hour;
            unsigned short current_day = (((map[0xC] & 0x1) << 8) | map[0xB]);
            unsigned short day = (t / (60 * 60 * 24) % 512);
            bool day_overflow = (current_day == 511 && day == 0);
            map[0xB] = day & 0xff;
            map[0xC] = ((day >> 8) | (map[0xC] & 0xfe));
            if (day_overflow)
                map[0xC] |= (1 << 7);
            else
                map[0xC] &= ~(1 << 7);
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

   if (index >= constants::EXTERNAL_RAM_BEGIN && index <= constants::EXTERNAL_RAM_END && external_ram_enabled && !external_ram.empty()) {
       if (current_mbc == MBC2) {
           index = (index % 512) + constants::EXTERNAL_RAM_BEGIN;
           value &= 0xf;
       }
        auto current_address = index - constants::EXTERNAL_RAM_BEGIN;
        if (current_address + (current_external_ram_n * 8192) <= external_ram.size())
            external_ram[current_address + (current_external_ram_n * 8192)] = value;
       return;
   }

    if (index == constants::DIV_REGISTER){
        value = 0;
        div_written = true;
    }

    if (index == constants::TIMA_REGISTER) {
        if (tima_overflow) return;
        tima_written = true;
    }

    if (index == constants::TMA_REGISTER) {
        tma_written = true;
    }

    if (index == constants::TAC_REGISTER) {
        tac_written = true;
    }

    update_channels_triggers(index, value);

    if (index == constants::NR52_ADDRESS && !((value >> 7) & 1) && (map[constants::NR52_ADDRESS] >> 7) & 1)
        apu_power_off();

    if (std::any_of(std::begin(apu_regs), std::end(apu_regs),
                    [index](auto& el){return el == index && el != constants::NR52_ADDRESS;}) &&
                    !((map[constants::NR52_ADDRESS] >> 7) & 1))
        return;

    if (index == constants::NR52_ADDRESS) {
        value &= 0xf0;
        value |= map[constants::NR52_ADDRESS] & 0xf;
    }

#if CHEATS_ON
    if (CheatCore::is_frozen(index))
        return;
#endif

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

    if (index >= constants::EXTERNAL_RAM_BEGIN && index <= constants::EXTERNAL_RAM_END) {
        // todo: maybe do (res | 0xF0) for MBC2 type
        return external_ram_enabled ? external_ram[(index - constants::EXTERNAL_RAM_BEGIN) + (current_external_ram_n * 8192)] : dummy_reg;
    }

    if (index >= constants::CH3_WAVES_BASE && index <= constants::CH3_WAVES_BASE + 15){
        if (ch3_read_enable)
            index = constants::CH3_WAVES_BASE + ch3_sample_index;
        else
            return dummy_reg;
    }

    if (is_ram_avaliable(index)){
        for (int i = 0; auto el : apu_regs){
            if (el == index){
                return map[index] | apu_regs_masks[i];
            }
            ++i;
        }
        return map[index];
    }

    return dummy_reg;
}

void Memory::apu_power_off() {
    for (auto index: apu_regs) {
        if (index == constants::NR52_ADDRESS)
            continue;
        map[index] = 0;
    }
}

void Memory::write_unsafe(unsigned short index, unsigned char value) {
    if (index >= constants::ROM_BANK_00_BEGIN && index <= constants::ROM_BANK_00_END){
        rom[(current_rom_bank0 * constants::BANK_ROM_SZ) + index] = value;
        return;
    }

    if (index >= constants::ROM_BANK_01_BEGIN && index <= constants::ROM_BANK_01_END) {
        rom[(current_rom_bank1 * constants::BANK_ROM_SZ) + (index - constants::ROM_BANK_01_BEGIN)] = value;
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

    if (index == constants::DIV_REGISTER){
        value = 0;
        div_written = true;
    }

    if (index == constants::TAC_REGISTER) {
        tac_written = true;
    }

    if (index >= constants::EXTERNAL_RAM_BEGIN && index <= constants::EXTERNAL_RAM_END) {
        if (current_mbc == MBC2) {
            index = (index % 512) + constants::EXTERNAL_RAM_BEGIN;
            value &= 0xf;
        }
        auto current_address = index - constants::EXTERNAL_RAM_BEGIN;
        external_ram[current_address + (current_external_ram_n * 8192)] = value;
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

    if (index >= constants::EXTERNAL_RAM_BEGIN && index <= constants::EXTERNAL_RAM_END) {
        // todo: maybe do (res | 0xF0) for MBC2 type
        return external_ram_enabled ? external_ram[(index - constants::EXTERNAL_RAM_BEGIN) + (current_external_ram_n * 8192)] : dummy_reg;
    }
    return map[index];
}

void Memory::update_channels_triggers(unsigned short address, unsigned char value) {
    if (address == constants::NR14_ADDRESS){
        ch1_trigger = (((value >> 7) & 1));
        ch1_enable_length_trigger = (((value >> 6) & 1)) && !((map[address] >> 6) & 1);
    } else if (address == constants::NR24_ADDRESS) {
        ch2_trigger = (((value >> 7) & 1));
        ch2_enable_length_trigger = (((value >> 6) & 1)) && !((map[address] >> 6) & 1);
    } else if (address == constants::NR34_ADDRESS) {
        ch3_trigger = ((value >> 7) & 1);
        ch3_enable_length_trigger = (((value >> 6) & 1)) && !((map[address] >> 6) & 1);
    } else if (address == constants::NR44_ADDRESS) {
        ch4_trigger = ((value >> 7) & 1);
        ch4_enable_length_trigger = (((value >> 6) & 1)) && !((map[address] >> 6) & 1);
    } else if (address == constants::NR11_ADDRESS)
        ch1_update_timer = true;
    else if (address == constants::NR21_ADDRESS)
        ch2_update_timer = true;
    else if (address == constants::NR31_ADDRESS)
        ch3_update_timer = true;
    else if (address == constants::NR41_ADDRESS)
        ch4_update_timer = true;
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
