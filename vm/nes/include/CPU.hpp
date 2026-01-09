#include "Constants.hpp"
#include "Bus.hpp"

struct CPU {

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
        unsigned char map[0x10000]{};

        unsigned char* read_addr(Bus&, unsigned short addr);
        unsigned char read(Bus&, unsigned short addr) const;
        void write(Bus&, unsigned short addr, unsigned char value);
        static void adjust_if_mirror(unsigned short&);
    } mem;

    inline static unsigned short write_mmio_addr[] = {
            constants::PPUCTRL, constants::PPUMASK, constants::OAMADDR, constants::OAMDATA,
            constants::PPUSCROLL, constants::PPUADDR, constants::PPUDATA, constants::OAMDMA
    };

    inline static unsigned short read_mmio_addr[] = {
            constants::PPUSTATUS,  constants::OAMDATA,constants::PPUDATA,
    };




// https://www.nesdev.org/NESDoc.pdf (page 12-13)
    // SPR
    unsigned short pc{}; 
    unsigned char sp = 0xFF;
    struct StatusReg {
        // | N | V |   | B | D | I | Z | C |
        // | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
        unsigned char reg{};
        enum Flag: unsigned char {
            C = 0,
            Z = 1,
            I = 2,
            D = 3,
            B = 4,
            Unused = 5,
            V = 6,
            N = 7
        };
        bool get(Flag);
        void set(Flag flag, bool value);
        unsigned char get_status() const;
        void set_status(unsigned char v);
    } sr;
                       
    // GPR
    unsigned char a{};
    unsigned char x{};
    unsigned char y{};

    enum Interrupt: unsigned char {
        RESET, NMI, IRQ, InterruptsCount
    };

    // request RESET interrupt on start (https://www.nesdev.org/NESDoc.pdf - page 13)
    bool interrupt_requests[3]{true,false,false};
    unsigned short interrupt_addresses[3] = {
            constants::RESET_INTERRUPT_HANDLER_L,
            constants::NMI_INTERRUPT_HANDLER_L,
            constants::IRQ_INTERRUPT_HANDLER_L,
    };


    // https://www.nesdev.org/wiki/CPU_addressing_modes
    enum AddressingMode {
        ZeroPage,
        ZeroPageX,
        ZeroPageY,
        Absolute,
        AbsoluteX,
        AbsoluteY,
        IndexedIndirectX,
        IndexedIndirectY,
        IndexedZeroPage,
        IndexedAbsolute,
        Indirect,
        Implied,
        Accumulator,
        Immediate,
        Relative,
        IndirectIndexed,
    };

    struct AddressingResult {
        unsigned char* value{};
        bool page_crossed{};
    };

    AddressingResult get_arg(Bus& mem, AddressingMode am, bool cross_check);

    enum Instruction: unsigned char {
        ADC, AND, ASL,BCC, BCS,	BEQ, BIT, BMI, BNE,	BPL, BRK, BVC, BVS,	CLC,
        CLD, CLI, CLV,CMP, CPX,	CPY, DEC, DEX, DEY,	EOR, INC, INX, INY,	JMP,
        JSR, LDA, LDX,LDY, LSR,	NOP, ORA, PHA, PHP,	PLA, PLP, ROL, ROR,	RTI,
        RTS, SBC, SEC,SED, SEI,	STA, STX, STY, TAX,	TAY, TSX, TXA, TXS,	TYA,
    };

    struct DecodedInstruction {
        AddressingMode addr_mode;
        Instruction i;
        unsigned char bytes;
        unsigned char cycles;
    };

    DecodedInstruction decode(unsigned char v);
    void execute(Bus& mem, DecodedInstruction di);
    void handle_interrupt(Bus&, Interrupt);
};
