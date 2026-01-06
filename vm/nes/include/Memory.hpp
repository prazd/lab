struct Memory {
// https://www.nesdev.org/wiki/CPU_memory_map
//    $0000–$07FF	$0800	2 KB internal RAM
//    $0800–$0FFF	$0800
//    $1000–$17FF	$0800   Mirrors of $0000–$07FF
//    $1800–$1FFF	$0800
//    $2000–$2007	$0008	NES PPU registers
//    $2008–$3FFF	$1FF8	Mirrors of $2000–$2007 (repeats every 8 bytes)
//    $4000–$4017	$0018	NES APU and I/O registers
//    $4018–$401F	$0008	APU and I/O functionality that is normally disabled. See CPU Test Mode.
//    $4020–$FFFF   $BFE0   Unmapped. Available for cartridge use.
//    • $6000–$7FFF $2000   Usually cartridge RAM, when present.
//    • $8000–$FFFF $8000   Usually cartridge ROM and mapper registers.
    unsigned char cpu_map[0x10000]{};

// https://www.nesdev.org/wiki/PPU_memory_map
//  $0000-$0FFF	$1000	Pattern table 0	Cartridge
//  $1000-$1FFF	$1000	Pattern table 1	Cartridge
//  $2000-$23BF	$03c0	Nametable 0	Cartridge
//  $23C0-$23FF	$0040	Attribute table 0	Cartridge
//  $2400-$27BF	$03c0	Nametable 1	Cartridge
//  $27C0-$27FF	$0040	Attribute table 1	Cartridge
//  $2800-$2BBF	$03c0	Nametable 2	Cartridge
//  $2BC0-$2BFF	$0040	Attribute table 2	Cartridge
//  $2C00-$2FBF	$03c0	Nametable 3	Cartridge
//  $2FC0-$2FFF	$0040	Attribute table 3	Cartridge
//  $3000-$3EFF	$0F00	Table Mirror $2000-$2EFF
//  $3F00-$3F1F	$0020	Palette RAM indexes	Internal to PPU
//  $3F20-$3FFF	$00E0	Mirrors of $3F00-$3F1F	Internal to PPU
    unsigned char ppu_map[0x4000]{};
    unsigned char ppu_oam[0x100]{};

    bool nmi_interrupt_flip_flop = false;

    struct PPUREG {
        unsigned char reg{};
        bool was_written{};
    };

    PPUREG ppuctrl{};
    PPUREG ppumask{};
    PPUREG ppustatus{};
    PPUREG oamaddr{};
    PPUREG oamdata{};
    PPUREG ppuscroll{};
    PPUREG ppuaddr{};
    PPUREG ppudata{};

    unsigned char dma_source_high_byte{};

    unsigned char* cpu_read_addr(unsigned short addr);
    unsigned char cpu_read(unsigned short addr) const;
    void cpu_write(unsigned short addr, unsigned char value);
    static void cpu_adjust_if_mirror(unsigned short&);

    unsigned char ppu_read(unsigned short addr) const;
    void ppu_write(unsigned short addr, unsigned char value);
    static void ppu_adjust_if_mirror(unsigned short&);
};
