#include "RAM.h"

void RAM::update_rom_bank01(size_t bank) {
    current_rom_bank = bank;
}

void RAM::update_cart_ram(size_t n) {
    current_cart_ram = n;
}

bool RAM::lcd_enabled() const {
    return mem[constants::LCDC_ADDRESS] & 0x80;
}

void RAM::handle_mbc(unsigned short index, unsigned char value) {
    switch (current_mbc) {
        case MBC0: {handle_mbc0(index, value); break;}
        case MBC1: {handle_mbc1(index, value); break;}
    }
}

void RAM::handle_mbc0(unsigned short index, unsigned char value) {
    if (index >= 0x0000 && index <= 0x1FFF) external_ram_enabled = (value & 0xf) == 0x0A;
}

void RAM::handle_mbc1(unsigned short index, unsigned char value) {
    // MBC-1
    // https://hacktix.github.io/GBEDG/mbcs/mbc1/
    if (index >= 0x0000 && index <= 0x1FFF) {
        external_ram_enabled = (value & 0xf) == 0x0A;
    } else if (index >= 0x2000 && index <= constants::ROM_BANK_00_END) {
        auto v = value & 0x1f;
        if (v == 0) v = 1;
        update_rom_bank01(v);
    } else if (index >= constants::ROM_BANK_01_BEGIN && index <= 0x5FFF) {
        auto v = value & 0x3;
        if (current_banking_mode == Simple) {
            update_rom_bank01(v << 5 | current_rom_bank & 0x1f);
        } else {
            update_cart_ram(v); // todo: check
        }
    } else if (index >= 0x6000 && index <= constants::ROM_BANK_01_END) {
        if (value & 1) current_banking_mode = Advanced;
        else current_banking_mode = Simple;
    }
}

bool RAM::is_ram_avaliable(unsigned short index) const {
    auto current_ppu_mode = get_ppu_mode();
    if (((index >=  constants::VRAM_BEGIN && index <=  constants::VRAM_END)  && (current_ppu_mode == 3 && lcd_enabled())) ||
        ((index >=  constants::EXTERNAL_RAM_BEGIN && index <=  constants::EXTERNAL_RAM_END)) && (!external_ram_enabled) || // todo: check external RAM
        ((index >=  constants::OAM_BEGIN && index <=  constants::OAM_END)) && ((current_ppu_mode == 2 || current_ppu_mode == 3) && lcd_enabled()) ||
        ((index >=  constants::NOT_USABLE_BEGIN && index <=  constants::NOT_USABLE_END))
            ) return false;
    return true;
}

unsigned char RAM::get_ppu_mode() const {
    return mem[constants::STAT_ADDRESS] & 0x03;
}

void RAM::write(unsigned short index, unsigned char value) {
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

    if (is_ram_avaliable(index))
        mem[index] = value;
}

unsigned char RAM::read(unsigned short index) const {
    if (index >= constants::OAM_BEGIN && index <= constants::OAM_END && dma_active){
        return dummy_reg;
    }

    if (index >= constants::ROM_BANK_00_BEGIN && index <= constants::ROM_BANK_00_END){
        return rom[index];
    }

    if (index >= constants::ROM_BANK_01_BEGIN && index <= constants::ROM_BANK_01_END) {
        return rom[(current_rom_bank * constants::BANK_ROM_SZ) + (index - constants::ROM_BANK_01_BEGIN)];
    }

    if (index >= constants::ECHO_RAM_BEGIN && index <= constants::ECHO_RAM_END){
        std::cout << "ECHO RAM READ!" << '\n';
        index -= 0x2000;
    }

    if (index == constants::DIV_REGISTER){
        return div_internal_sync >> 8;
    }

    if (index >= constants::EXTERNAL_RAM_BEGIN && index <= constants::EXTERNAL_RAM_END) {
        return external_ram_enabled ? cart_ram[(index - constants::EXTERNAL_RAM_BEGIN) + (current_cart_ram * 8192)] : dummy_reg;
    }

    if (is_ram_avaliable(index))
        return mem[index];

//    std::cout << "DUMMY_REG!!!" << '\n';
    return dummy_reg;
}

void RAM::write_unsafe(unsigned short index, unsigned char value) {
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
        bool old_enabled = (mem[constants::TAC_REGISTER] >> 2) & 1;
        auto old_value = mem[constants::TAC_REGISTER];
        mem[constants::TAC_REGISTER] = value;

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
            mem[constants::TIMA_REGISTER] =  prev + 1;
            if (prev == 0xff) {
                mem[constants::IF_ADDRESS] =  read(constants::IF_ADDRESS) | (1 << 2);
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

    mem[index] = value;
}

unsigned char RAM::read_unsafe(unsigned short index) const {
    if (index >= constants::ROM_BANK_00_BEGIN && index <= constants::ROM_BANK_00_END){
        return rom[index];
    }

    if (index >= constants::ROM_BANK_01_BEGIN && index <= constants::ROM_BANK_01_END) {
        return rom[(current_rom_bank * constants::BANK_ROM_SZ) + (index - constants::ROM_BANK_01_BEGIN)];
    }

    if (index >= constants::ECHO_RAM_BEGIN && index <= constants::ECHO_RAM_END){
        index -= 0x2000;
    }

    if (index == constants::DIV_REGISTER){
        return div_internal_sync >> 8;
    }

    if (index >= constants::EXTERNAL_RAM_BEGIN && index <= constants::EXTERNAL_RAM_END) {
        return cart_ram[(index - constants::EXTERNAL_RAM_BEGIN) + (current_cart_ram * 8192)];
    }
    return mem[index];
}

void RAM::dma_tick() {
    if (dma_active) {
        ++dma_t;
        if (dma_t % 4 == 0) {
            // mem[constants::OAM_BEGIN + dma_counter] = mem[dma_address + dma_counter];
            write_unsafe(constants::OAM_BEGIN + dma_counter, read_unsafe(dma_address + dma_counter));
            ++dma_counter;
            if (dma_counter == 160) {
                dma_active = false;
                dma_address = 0;
                dma_counter = 0;
            }
        }
    }
}
