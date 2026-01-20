#ifndef NES_MAPPERS_H
#define NES_MAPPERS_H
#include <vector>
namespace mappers {
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

        inline void handle(unsigned short address, unsigned char val) {
            if (val & 0x80) {
                mmc1_sr = 0x10;
                control_register = 0; // todo: check
                return;
            }

            // in progress
            if (!(mmc1_sr & 1)) {
                mmc1_sr >>= 1;
                mmc1_sr |= ((val & 1) << 4);
                return;
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
        };

        // todo: check MMC1A/MMC1B
        inline unsigned char* read_rom(std::vector<unsigned char>& rom, unsigned short addr){
            unsigned short local_addr = addr-0x8000;
            switch ((control_register >> 2) & 0x3) {
                case 0b00:
                case 0b01: return &rom[((prg_bank & 0xe) * 0x4000) + local_addr]; // 0,2,4...
                case 0b10: {
                    // first 16kb (0) lock
                    if (local_addr < 0x4000) return &rom[local_addr];
                    return &rom[(local_addr-0x4000) + (0x4000 * (prg_bank & 0xf))];
                }
                default: {
                    // last 16kb (sz-1) lock
                    if (local_addr >= 0x4000 && local_addr < 0x8000) return &rom[(local_addr-0x4000) + (0x4000 * ((rom.size()/0x4000)-1))];
                    return &rom[local_addr + (0x4000 * (prg_bank & 0xf))];
                }
            }
        }
    };
}

struct Mapper {
    enum MapperType {
        MMC1 = 0x1
    } mtype = MMC1;

    std::vector<unsigned char> ram;
    std::vector<unsigned char> rom;

    mappers::MMC1 mmc1;

    inline unsigned char* read_rom(unsigned short addr){
        if (mtype == MMC1) return mmc1.read_rom(rom, addr);
        return nullptr;
    }
};

#endif //NES_MAPPERS_H
