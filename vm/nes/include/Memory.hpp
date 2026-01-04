// https://www.nesdev.org/wiki/CPU_memory_map
// $0000–$07FF 2 KB internal RAM
// $0800–$0FFF
// $1000–$17FF  - Mirrors of $0000–$07FF
// $1800–$1FFF
// $2000–$2007 - PPU registers
// $2008–$3FFF - Mirrors of $2000–$2007 (repeats every 8 bytes)
// $4000–$4017 - APU and I/O registers
// $4018–$401F - APU and I/O functionality that is normally disabled
// $4020–$FFFF - Unmapped. Available for cartridge use
//   - $6000–$7FFF - Usually cartridge RAM, when present
//   - $8000–$FFFF - Usually cartridge ROM and mapper registers

struct Memory {
    unsigned char map[0x10000];
    unsigned char* read_addr(unsigned short addr);
    unsigned char read(unsigned short addr) const;
    void write(unsigned short addr, unsigned char value);
    static void adjust_if_mirror(unsigned short&);
};
