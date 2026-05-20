#ifndef NES_MAPPERS_H
#define NES_MAPPERS_H
#include <vector>
#include <array>
#include "Constants.hpp"
namespace mappers {
    struct MMC3 {
        enum Regs {
            R0, R1, R2, R3, R4,R5,R6,R7,RegsCount
        } selected_bank = R0;
        bool a12_inversion = true;
        bool prg_banks_mode = true;
        bool irq_requested = false;

        unsigned char irq_latch{};
        unsigned char irq_counter{};
        bool reload_flag = false;
        unsigned short last_address{};
        bool irq_enable {};
        size_t last_write_cycles = 0;

        bool is_vertical = false;

        // todo: debug + finish
        std::array<unsigned char, RegsCount> regs_bank_value{};
        inline unsigned char read_prg_rom(const std::vector<unsigned char>& rom, unsigned short addr) {
            if (!prg_banks_mode){
                if (addr >= 0x8000 && addr <= 0x9fff){
                    return rom[(regs_bank_value[R6] * 8192) + (addr - 0x8000)];
                } else if (addr >= 0xa000 && addr <= 0xbfff){
                    return rom[(regs_bank_value[R7] * 8192) + (addr - 0xa000)];
                } else if (addr >= 0xc000 && addr <= 0xdfff){
                    return rom[((rom.size()/8192) - 2) * 8192 + (addr - 0xc000)];
                } else if (addr >= 0xe000 && addr <= 0xffff){
                    return rom[((rom.size()/8192) - 1) * 8192 + (addr - 0xe000)];
                }
            } else {
                if (addr >= 0x8000 && addr <= 0x9fff){
                    return rom[((rom.size()/8192) - 2) * 8192 + (addr - 0x8000)];
                } else if (addr >= 0xa000 && addr <= 0xbfff){
                    return rom[(regs_bank_value[R7] * 8192) + (addr - 0xa000)];
                } else if (addr >= 0xc000 && addr <= 0xdfff){
                    return rom[(regs_bank_value[R6] * 8192) + (addr - 0xc000)];
                } else if (addr >= 0xe000 && addr <= 0xffff){
                    return rom[((rom.size()/8192) - 1) * 8192 + (addr - 0xe000)];
                }
            }
            return 0;
        }

        inline void tick_irq(unsigned short addr){
                if ((addr >> 12) & 1 && !((last_address >> 12) & 1) && last_write_cycles >= 8){ // maybe add 8 ppu-cycles filter
                    if (irq_counter) --irq_counter;
                    if (!irq_counter) {
                        if (reload_flag) {
                            reload_flag = false;
                        } else {
                            if (irq_enable)
                                irq_requested = true;
                        }
                        irq_counter = irq_latch;
                    }
                }
                last_write_cycles = 0;
                last_address = addr;
        }

        inline void tick_irq(unsigned short addr, unsigned short prev_addr){
            if ((addr >> 12) & 1 && !((prev_addr >> 12) & 1) && last_write_cycles >= 8){ // maybe add 8 ppu-cycles filter
                if (irq_counter) --irq_counter;
                if (!irq_counter) {
                    if (reload_flag) {
                        reload_flag = false;
                    } else {
                        if (irq_enable)
                            irq_requested = true;
                    }
                    irq_counter = irq_latch;
                }
            }
            last_write_cycles = 0;
            last_address = addr;
        }

        inline void reset() {
            last_address = 0;
            irq_counter = 0;
            irq_latch = 0;
            irq_requested = false;
            a12_inversion = true;
            prg_banks_mode = true;
            irq_requested = false;
            is_vertical = false;
            selected_bank = R0;
            regs_bank_value = {};
        }

        inline unsigned char read_chr_rom(const std::vector<unsigned char>& rom, unsigned short addr) {
            if (!a12_inversion) {
                if (addr >= 0x0000 && addr <= 0x07ff){
                    return rom[((regs_bank_value[R0] & 0xfe) * 1024) + addr];
                } else if (addr >= 0x800 && addr <= 0xfff) {
                    return rom[((regs_bank_value[R1] & 0xfe) * 1024) + (addr - 0x800)];
                } else if (addr >= 0x1000 && addr <= 0x13ff){
                    return rom[(regs_bank_value[R2] * 1024) + (addr - 0x1000)];
                } else if (addr >= 0x1400 && addr <= 0x17ff){
                    return rom[(regs_bank_value[R3] * 1024) + (addr - 0x1400)];
                } else if (addr >= 0x1800 && addr <= 0x1bff) {
                    return rom[(regs_bank_value[R4] * 1024) + (addr - 0x1800)];
                } else if (addr >= 0x1c00 && addr <= 0x1fff) {
                    return rom[(regs_bank_value[R5] * 1024) + (addr - 0x1c00)];
                }
            } else {
                if (addr >= 0x1000 && addr <= 0x17ff){
                    return rom[((regs_bank_value[R0] & 0xfe) * 1024) + (addr-0x1000)];
                } else if (addr >= 0x1800 && addr <= 0x1fff){
                    return rom[((regs_bank_value[R1] & 0xfe) * 1024) + (addr-0x1800)];
                } else if (addr >= 0x0000 && addr <= 0x3ff) {
                    return rom[(regs_bank_value[R2] * 1024) + addr];
                } else if (addr >= 0x400 && addr <= 0x7ff){
                    return rom[(regs_bank_value[R3] * 1024) + (addr-0x400)];
                } else if (addr >= 0x800 && addr <= 0xbff) {
                    return rom[(regs_bank_value[R4] * 1024) + (addr-0x800)];
                } else if (addr >= 0xc00 && addr <= 0xfff) {
                    return rom[(regs_bank_value[R5] * 1024) + (addr-0xc00)];
                }
            }
            return 0;
        }
        inline void handle(unsigned short addr, unsigned char value){
            if (addr >= 0x8000 && addr <= 0x9FFF){
                if (addr & 1){ // update bank
                    regs_bank_value[selected_bank] = value;
                } else { // select bank
                    selected_bank = static_cast<Regs>(value & 0x7);
                    a12_inversion = (value >> 7) & 1;
                    prg_banks_mode = (value >> 6) & 1;
                }
            } else if (addr >= 0xa000 && addr <= 0xbffe) {
                is_vertical = !(value & 1);
            } else if (addr >= 0xc000 && addr <= 0xDFFF) {
                if (addr & 1){
                    reload_flag = true;
                } else {
                    irq_latch = value;
                }
            } else if (addr >= 0xe000 && addr <= 0xffff) {
                irq_requested = false;
                irq_enable = addr & 1;
            }
        }
    };

    struct NROM {
        inline unsigned char read_prg_rom(const std::vector<unsigned char>& rom, unsigned short addr) {
            return rom[addr-0x8000];
        }
        inline unsigned char read_chr_rom(const std::vector<unsigned char>& rom, unsigned short addr) {
            return rom[addr];
        }
    };

    struct UXROM {
        unsigned char current_bank = 0;

        inline void handle(unsigned short addr, unsigned char value){
            current_bank = value & 0xf;
        }
        inline unsigned char read_prg_rom(const std::vector<unsigned char>& rom, unsigned short addr) {
            if (addr >= 0xC000 && addr <= 0xFFFF)
                return  rom[(rom.size()-0x4000) + addr-0xC000];
            return rom[addr-0x8000 + (current_bank * 0x4000)];
        }
        inline unsigned char read_chr_rom(const std::vector<unsigned char>& rom, unsigned short addr) {
            return rom[addr];
        }
    };

    struct CNROM {
        unsigned short current_offset{};
        inline unsigned char read_prg_rom(const std::vector<unsigned char>& rom, unsigned short addr) {
            return rom[addr-0x8000];
        }
        inline unsigned char read_chr_rom(const std::vector<unsigned char>& rom, unsigned short addr) {
            return rom[(0x2000 * current_offset) + addr];
        }
        inline void handle(unsigned short addr, unsigned char value){
            current_offset = value;
        }
    };

    struct MMC1 {
        //  CPU $6000-$7FFF: 8 KB PRG-RAM bank, (optional)
        //  CPU $8000-$BFFF: 16 KB PRG-ROM bank, either switchable or fixed to the first bank
        //  CPU $C000-$FFFF: 16 KB PRG-ROM bank, either fixed to the last bank or switchable
        //  PPU $0000-$0FFF: 4 KB switchable CHR bank
        //  PPU $1000-$1FFF: 4 KB switchable CHR bank
        unsigned char mmc1_sr = 0x10;       // 5 bit
        unsigned char load_register = 0;    // $8000-$FFFF
        unsigned char control_register = 0; // $8000-$9FFF
        unsigned char chr_bank0 = 0;        // $A000-$BFFF
        unsigned char chr_bank1 = 0;        // $C000-$DFFF
        unsigned char prg_bank = 0;         // $E000-$FFFF

        inline bool handle(unsigned short address, unsigned char val) {
            if (val & 0x80) {
                mmc1_sr = 0x10;
                control_register = 0; // todo: check
                return false;
            }

            // in progress
            if (!(mmc1_sr & 1)) {
                mmc1_sr >>= 1;
                mmc1_sr |= ((val & 1) << 4);
                return false;
            }

            mmc1_sr >>= 1;
            mmc1_sr |= ((val & 1) << 4);

            if (address >= 0x8000 && address <= 0x9fff) {
                control_register = mmc1_sr;
            } else if (address >= 0xa000 && address <= 0xbfff) {
                chr_bank0 = mmc1_sr;
            } else if (address >= 0xc000 && address <= 0xdfff) {
                chr_bank1 = mmc1_sr;
            } else if (address >= 0xe000 && address <= 0xffff) {
                prg_bank = mmc1_sr;
            }
            mmc1_sr = 0x10;
            return true;
        };

        inline unsigned char get_screen_mirroring_value() const {
            return control_register & 0x3;
        }

        // todo: check MMC1A/MMC1B
        inline unsigned char read_prg_rom(std::vector<unsigned char>& rom, unsigned short addr){
            unsigned short local_addr = addr-0x8000;
            switch ((control_register >> 2) & 0x3) {
                case 0b00:
                case 0b01: return rom[((prg_bank & 0xe) * 0x4000) + local_addr]; // 0,2,4...
                case 0b10: {
                    // first 16kb (0) lock
                    if (local_addr < 0x4000) return rom[local_addr];
                    return rom[(local_addr-0x4000) + (0x4000 * (prg_bank & 0xf))];
                }
                default: {
                    // last 16kb (sz-1) lock
                    if (local_addr >= 0x4000 && local_addr < 0x8000) return rom[(local_addr-0x4000) + (0x4000 * ((rom.size()/0x4000)-1))];
                    return rom[local_addr + (0x4000 * (prg_bank & 0xf))];
                }
            }
        }
        inline unsigned char read_chr_rom(const std::vector<unsigned char>& rom, unsigned short addr){
            // todo: check
            return rom[addr];
        }
    };
}

struct Mapper {
    enum MapperType {
        NROM = 0x0,
        MMC1 = 0x1,
        UxROM = 0x2,
        CNROM = 0x3,
        MMC3 = 0x4
    } mtype = MMC1;

    enum ScreenMirroring {
        Vertical,
        Horizontal,
        SingleScreenLower,
        SingleScreenUpper,
        MirroringCount
    } current_screen_mirroring;

    bool is_chr_ram = false;

    VEC_MODE std::vector<unsigned char> prg{};
    VEC_MODE std::vector<unsigned char> chr{}; // rom/ram

    mappers::NROM nrom;
    mappers::MMC1 mmc1;
    mappers::CNROM cnrom;
    mappers::UXROM uxrom;
    mappers::MMC3 mmc3;

    inline void update_screen_mirroring(bool is_horizontal_arrangement, bool is_alternative_nametable) {
        switch (mtype) {
            case MMC1: {
                switch (mmc1.get_screen_mirroring_value()) {
                    case 0b00: current_screen_mirroring = SingleScreenLower; break;
                    case 0b01: current_screen_mirroring = SingleScreenUpper; break;
                    case 0b10: current_screen_mirroring = Vertical;          break;
                    case 0b11: current_screen_mirroring = Horizontal;        break;
                }
                break;
            }
            case MMC3: {
                current_screen_mirroring = mmc3.is_vertical ? Vertical: Horizontal;
                break;
            }
            default: {
                current_screen_mirroring = is_horizontal_arrangement ? Horizontal : Vertical;
                break;
            }
        }
    }

    inline unsigned char read_prg_rom(unsigned short addr){
        switch (mtype) {
            case NROM: return nrom.read_prg_rom(prg, addr);
            case MMC1: return mmc1.read_prg_rom(prg, addr);
            case UxROM: return uxrom.read_prg_rom(prg, addr);
            case CNROM: return cnrom.read_prg_rom(prg, addr);
            case MMC3: return mmc3.read_prg_rom(prg, addr);
            default: return 0xff;
        }
    }

    inline unsigned char read_chr_rom(unsigned short addr){
        switch (mtype) {
            case NROM:   return nrom.read_chr_rom(chr, addr);
            case MMC1: { return mmc1.read_chr_rom(chr, addr);}
            case UxROM: { return uxrom.read_chr_rom(chr, addr);}
            case CNROM:  return cnrom.read_chr_rom(chr, addr);
            case MMC3: return mmc3.read_chr_rom(chr,addr);
            default: return 0xff;
        }
    }

    inline bool is_irq_requested() {
        return mmc3.irq_requested;
    }

    inline void write_chr_ram(unsigned short addr, unsigned char value){
        // todo: check
        if (mtype != CNROM)
            chr[addr] = value;
    }

    inline void handle(unsigned short addr, unsigned char value){
        switch (mtype) {
            case MMC1: {
                if (mmc1.handle(addr, value)) update_screen_mirroring(false,false); break;}
            case UxROM:
                uxrom.handle(addr, value); break;
            case CNROM: {cnrom.handle(addr, value);  break;}
            case MMC3:  {mmc3.tick_irq(addr); mmc3.handle(addr, value); update_screen_mirroring(false, false); break;}
            default: return;
        }
    }
    inline void update_state(unsigned short last_v, unsigned short prev_v){
        if (mtype == MMC3){
            mmc3.tick_irq(last_v, prev_v);
        }
    }
    inline void reset(){
        if (mtype == MMC3){
            mmc3.reset();
        }
    }
    inline void tick(){
        if (mtype == MMC3) {
            ++mmc3.last_write_cycles;
        }
    }
};

#endif //NES_MAPPERS_H
