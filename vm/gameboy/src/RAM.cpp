#include "RAM.h"

void RAM::update_rom_bank01(size_t bank) {
    current_rom_bank = bank;
    memcpy(mem + constants::BANK_ROM_SZ, rom.data() + (constants::BANK_ROM_SZ * current_rom_bank), constants::BANK_ROM_SZ);
}

void RAM::update_cart_ram(size_t n) {
    current_cart_ram = n;
    memcpy(mem + 0xA000, cart_ram.data() + (constants::EXTERNAL_RAM_SZ * current_cart_ram), constants::EXTERNAL_RAM_SZ);
}

bool RAM::lcd_enabled() const {
    return mem[0xFF00 + 0x40] & 0x80;
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
            update_cart_ram(v);
        }
    } else if (index >= 0x6000 && index <= constants::ROM_BANK_01_END) {
        if (value & 1) current_banking_mode = Advanced;
        else current_banking_mode = Simple;
    }
}

bool RAM::is_ram_avaliable(unsigned short index) const {
    auto current_ppu_mode = get_ppu_mode();
    if (((index >=  constants::VRAM_BEGIN && index <=  constants::VRAM_END)  && (current_ppu_mode == 3 && lcd_enabled())) ||
        ((index >=  constants::EXTERNAL_RAM_BEGIN && index <=  constants::EXTERNAL_RAM_END)) && (cart_ram.empty() || !external_ram_enabled) || // todo: check external RAM
        ((index >=  constants::OAM_BEGIN && index <=  constants::OAM_END)) && ((current_ppu_mode == 2 || current_ppu_mode == 3) && lcd_enabled()) ||
        ((index >=  constants::NOT_USABLE_BEGIN && index <=  constants::NOT_USABLE_END))
            ) return false;
    return true;
}

unsigned char RAM::get_ppu_mode() const {
    return mem[constants::STAT_ADDRESS] & 0x03;
}

void RAM::write(unsigned short index, unsigned char value) {
//    if (index >= constants::VRAM_BEGIN && index <= constants::VRAM_END) {
//        std::cout << "Writing to VRAM: Address 0x" << std::hex << index << " Value 0x" << (unsigned)value << std::dec << '\n';
//    }
//    std::cout << std::dec;
    if (index <=  constants::ROM_BANK_01_END){
        handle_mbc(index, value);
        return;
    }
    if (index >= constants::ECHO_RAM_BEGIN && index <= constants::ECHO_RAM_END)
        index -= 0x2000;

    if (is_ram_avaliable(index))
        mem[index] = value;
}

unsigned char RAM::read(unsigned short index) const {
    if (index >= constants::ECHO_RAM_BEGIN && index <= constants::ECHO_RAM_END)
        index -= 0x2000;
    if (is_ram_avaliable(index))
        return mem[index];
    return dummy_reg;
}