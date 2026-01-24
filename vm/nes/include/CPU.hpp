#include "Constants.hpp"
#include "Bus.hpp"
#include "SmallString.h"

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



    // For debug
    inline static unsigned short write_mmio_addr[] = {
            constants::PPUCTRL,
            constants::PPUMASK,
            constants::PPUSTATUS,
            constants::OAMADDR,
            constants::OAMDATA,
            constants::PPUSCROLL,
            constants::PPUADDR,
            constants::PPUDATA,
            constants::OAMDMA,
            constants::CONTROLLER1
    };

    inline static unsigned short read_mmio_addr[] = {
            constants::PPUCTRL,
            constants::PPUMASK,
            constants::PPUSTATUS,
            constants::OAMADDR,
            constants::OAMDATA,
            constants::PPUSCROLL,
            constants::PPUADDR,
            constants::PPUDATA,
            constants::OAMDMA,
            constants::CONTROLLER1
    };

    // https://www.nesdev.org/NESDoc.pdf (page 12-13)
    // SPR
    unsigned short pc{};
    unsigned char sp = 0xFD; // https://www.nesdev.org/wiki/CPU_power_up_state
    struct StatusReg {
        // | N | V |   | B | D | I | Z | C |
        // | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
        unsigned char reg = 0b0000'0000; //
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
        unsigned short addr{};
        bool page_crossed{};
        bool is_accumulator = false;
    };

    unsigned char read(Bus& bus, AddressingResult ar);
    void write(Bus& bus, AddressingResult ar, unsigned char value);

    AddressingResult get_arg_pc(Bus& bus, AddressingMode am, bool cross_check);
    AddressingResult get_arg_by_addr(Bus& bus, AddressingMode am, unsigned short pc_addr);

    enum Instruction: unsigned char {
        ADC, AND, ASL,BCC, BCS,	BEQ, BIT, BMI, BNE,	BPL, BRK, BVC, BVS,	CLC,
        CLD, CLI, CLV,CMP, CPX,	CPY, DEC, DEX, DEY,	EOR, INC, INX, INY,	JMP,
        JSR, LDA, LDX,LDY, LSR,	NOP, ORA, PHA, PHP,	PLA, PLP, ROL, ROR,	RTI,
        RTS, SBC, SEC,SED, SEI,	STA, STX, STY, TAX,	TAY, TSX, TXA, TXS,	TYA,
    };


    static inline std::string_view instruction_str[56] = {
        "ADC", "AND", "ASL","BCC", "BCS","BEQ", "BIT", "BMI", "BNE","BPL", "BRK", "BVC", "BVS",	"CLC",
        "CLD", "CLI", "CLV","CMP", "CPX","CPY", "DEC", "DEX", "DEY","EOR", "INC", "INX", "INY",	"JMP",
        "JSR", "LDA", "LDX","LDY", "LSR","NOP", "ORA", "PHA", "PHP","PLA", "PLP", "ROL", "ROR",	"RTI",
        "RTS", "SBC", "SEC","SED", "SEI","STA", "STX", "STY", "TAX","TAY", "TSX", "TXA", "TXS",	"TYA",
    };

    struct DecodedInstruction {
        AddressingMode addr_mode;
        Instruction i;
        unsigned char bytes;
        unsigned char cycles;
    };

    DecodedInstruction decode(unsigned char v);
    void tick(Bus& mem);
    bool execute_in_progress = false;
    bool interrupt_handling_in_progress = false;
    int interrupt_handling_cycle = 0;
    void handle_interrupt(Bus&, Interrupt);
    DecodedInstruction di{};

#if UI_DEBUGGER
    bool debug_run = true;
#endif

    struct AsmInfo {
        std::string_view mnemonic{};
        unsigned short operand{};
        bool is_accumulator{};
        bool no_operand{};
    };

    inline AsmInfo disassemble(Bus& bus, unsigned short address) {
        auto di = decode(mem.read(bus, address));
        switch (di.i) {
            case ADC:
            case AND:
            case ASL:
            case BIT:
            case CMP:
            case CPX:
            case CPY:
            case DEC:
            case EOR:
            case INC:
            case LDA:
            case LDX:
            case LDY:
            case LSR:
            case ORA:
            case SBC:
            {
                auto arg = get_arg_by_addr(bus, di.addr_mode, address);
                return {instruction_str[di.i], mem.read(bus, arg.addr), arg.is_accumulator, false};
            }

            case BCC:
            case BCS:
            case BEQ:
            case BMI:
            case BNE:
            case BPL:
            case BVC:
            case BVS:
            {
                auto arg = get_arg_by_addr(bus, di.addr_mode, address);
                return {instruction_str[di.i], (unsigned short)(address + (char)mem.read(bus, arg.addr) + di.bytes), arg.is_accumulator,
                        false};
            }

            case JSR:
            case STA:
            case STX:
            case STY:
            {
                auto arg = get_arg_by_addr(bus, di.addr_mode, address);
                return {instruction_str[di.i], arg.addr, arg.is_accumulator, false};
            }

            case BRK:
            case CLC:
            case CLD:
            case CLI:
            case CLV:
            case DEX:
            case DEY:
            case INX:
            case INY:
            case NOP:
            case PHA:
            case PHP:
            case PLA:
            case PLP:
            case ROL:
            case ROR:
            case RTI:
            case RTS:
            case SEC:
            case SED:
            case SEI:
            case TAX:
            case TAY:
            case TSX:
            case TXA:
            case TXS:
            case TYA:
                { return {instruction_str[di.i], 0, false, true}; }
            case JMP: {
                if (di.addr_mode == Absolute) {
                    return {instruction_str[di.i], (unsigned short)(mem.read(bus, address + 1) | (mem.read(bus, address + 2) << 8)), false,
                            false};
                } else { // Indirect
                    unsigned short arg_addr = (mem.read(bus, address + 1) | (mem.read(bus, address + 2) << 8));
                    unsigned char l = mem.read(bus, arg_addr);
                    unsigned char h = mem.read(bus, (arg_addr & 0xff) == 0xff ? (arg_addr & 0xff00) : arg_addr + 1);
                    return {instruction_str[di.i],(unsigned short)(l | (h << 8)), false , false};
                }
            }
        }
    }
};
