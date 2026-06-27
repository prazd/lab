#include "Constants.hpp"
#include "Bus.hpp"
#include "Simple.hpp"
#include <bitset>
struct CPU {
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

    // https://www.nesdev.org/NESDoc.pdf (page 12-13)
    // SPR
    unsigned short pc{};
    unsigned char sp = 0xFD; // https://www.nesdev.org/wiki/CPU_power_up_state
    struct StatusReg {
        // | N | V |   | B | D | I | Z | C |
        // | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
        unsigned char reg = 0b0000'0011; // todo: check
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

    size_t cycles_count = 0;

#if DEBUG_MODE
    unsigned short pc_watch_address{};
    size_t pc_watch_count{};

    bool debug_run = true;
#endif

    bool interrupt_requested = true;

    // request RESET interrupt on start (https://www.nesdev.org/NESDoc.pdf - page 13)
    bool interrupt_requests[3]{true,false,false};
    unsigned short interrupt_addresses[3] = {
            constants::RESET_INTERRUPT_HANDLER_L,
            constants::NMI_INTERRUPT_HANDLER_L,
            constants::IRQ_INTERRUPT_HANDLER_L,
    };

    // https://www.nesdev.org/wiki/CPU_addressing_modes
    enum AddressingMode: unsigned char{
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
        unsigned char add_to_addr{};
    } arg;
    void update_arg();

    unsigned char read(Bus& bus, AddressingResult ar);
    void write(Bus& bus, AddressingResult ar, unsigned char value);
    void reset();

    AddressingResult get_arg_by_addr(Bus& bus, AddressingMode am, unsigned short pc_addr);

    enum Instruction: unsigned char {
        ADC, AND, ASL,BCC, BCS,	BEQ, BIT, BMI, BNE,	BPL, BRK, BVC, BVS,	CLC,
        CLD, CLI, CLV,CMP, CPX,	CPY, DEC, DEX, DEY,	EOR, INC, INX, INY,	JMP,
        JSR, LDA, LDX,LDY, LSR,	NOP, ORA, PHA, PHP,	PLA, PLP, ROL, ROR,	RTI,
        RTS, SBC, SEC,SED, SEI,	STA, STX, STY, TAX,	TAY, TSX, TXA,
        //
        TXS, TYA,DOP, AAC, ASR, ARR, ATX, AXS, SLO, RLA, SRE, RRA,AAX,LAX,DCP,ISC,
        TOP, SYA, SXA
    };

    AddressingResult dummy_read(Bus& bus, AddressingResult);

    static constexpr inline std::string_view instruction_str[73] = {
        "ADC", "AND", "ASL","BCC", "BCS","BEQ", "BIT", "BMI", "BNE","BPL", "BRK", "BVC", "BVS",	"CLC",
        "CLD", "CLI", "CLV","CMP", "CPX","CPY", "DEC", "DEX", "DEY","EOR", "INC", "INX", "INY",	"JMP",
        "JSR", "LDA", "LDX","LDY", "LSR","NOP", "ORA", "PHA", "PHP","PLA", "PLP", "ROL", "ROR",	"RTI",
        "RTS", "SBC", "SEC","SED", "SEI","STA", "STX", "STY", "TAX","TAY", "TSX", "TXA", "TXS",	"TYA",
        "DOP", "AAC", "ASR", "ARR", "ATX", "AXS", "SLO", "RLA", "SRE", "RRA", "AAX", "LAX","DCP", "ISC",
        "TOP", "SYA", "SXA"
    };

    enum IPenaltyMode: unsigned char {
        NoPenaltyM = 0,
        CrossPageM = 1,
        DummyReadM = 2,
    };

    struct DecodedInstruction {
        AddressingMode addr_mode{};
        Instruction i{};
        unsigned char bytes{};
        unsigned char cycles{};
        IPenaltyMode penalty_mode = NoPenaltyM;
    };

    enum AddressingStep {
        LowByte, HighByte,
        Abs, Zp,
        AddrPlusX, AddrPlusY,
        AbsX, AbsY,
        IndXReadLow, IndXReadHigh, IndXRes,
        IndYReadLow, IndYReadHigh,
    };

    enum InstructionStep {
        Fetch,
        Execute,
    } istep = Execute;

    SimpleDequeTC<void(*)(Bus& bus, CPU&), 7> iop{};
    void iop_exec_step(Bus& bus);
    size_t current_iop_index{};

    struct InstructionState {
        unsigned short tmp_l{};
        unsigned short tmp_h{};
        unsigned char write_value{};
        unsigned char old_value{};
        unsigned char ind_low{};
        unsigned char addr_to_add{};
        unsigned short pc{};
        unsigned char arg1{};
        unsigned char arg2{};
        unsigned short result{};
        Interrupt interrupt = RESET;
        StatusReg::Flag flag = StatusReg::Flag::Unused;
    } as;

    void push_addr_step(AddressingStep);
    void fill_address_ops();
    void fill_exec_ops();

    DecodedInstruction decode(unsigned char v);
    void tick(Bus&);
    void tick_step(Bus&);
    void interrupt_handle(Bus&);
    void interrupt_handle_with_delay(Bus&);
    bool stop_requested = false;
    void push_interrupts_ops(Bus&);
    DecodedInstruction di{};

    template<StatusReg::Flag f, bool expect>
    void push_branch_ops() {
        iop.push_front([](Bus& bus, CPU& cpu){
            cpu.pc += cpu.di.bytes;
            if (cpu.sr.get(f) == expect){
                cpu.iop.push_front([](Bus& bus, CPU& cpu){
                    char add_val = (char)(cpu.read(bus, cpu.arg));
                    cpu.as.tmp_l = cpu.pc;
                    cpu.as.tmp_h = (cpu.pc & 0xff00) | ((cpu.pc + add_val) & 0xff);
                    unsigned char h = (cpu.pc >> 8) & 0xff;
                    cpu.pc += add_val;
                    bus.read_prg(cpu.as.tmp_l); // dummy read
                    if (h != ((cpu.pc >> 8) & 0xff)) {
                        ++cpu.di.cycles;
                        cpu.iop.push_back([](Bus &bus, CPU &cpu) {
                            bus.read_prg(cpu.as.tmp_h); // dummy read
                        }); // PageCross
                    }
                });
            }
        });
    };


//
    struct AsmInfo {
        std::string_view mnemonic{};
        unsigned short operand{};
        unsigned short arg_addr{};
        bool is_accumulator{};
        bool no_operand{};
        bool show_address{};
    };

    inline AsmInfo disassemble(Bus& bus, unsigned short address) {
        auto di = decode(bus.unsafe_read_prg(address));
        switch (di.i) {
            case SYA:
            case SXA:
            case ISC:
            case DCP:
            case LAX:
            case AAX:
            case RRA:
            case RLA:
            case SRE:
            case SLO:
            case AXS:
            case ATX:
            case ARR:
            case AAC:
            case ASR:
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
                return {instruction_str[di.i], bus.unsafe_read_prg(arg.addr), arg.addr, arg.is_accumulator, false, true};
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
                return {instruction_str[di.i], (unsigned short)(address + (char)bus.unsafe_read_prg(arg.addr) + di.bytes), arg.addr, arg.is_accumulator,
                        false, true};
            }

            case JSR:
            case STA:
            case STX:
            case STY:
            {
                auto arg = get_arg_by_addr(bus, di.addr_mode, address);
                return {instruction_str[di.i], arg.addr, arg.addr, arg.is_accumulator, false, true};
            }

            case TOP:
            case DOP:
            case TXS:
            case TYA:
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
                { return {instruction_str[di.i], 0, 0, false, true, false}; }
            case JMP: {
                if (di.addr_mode == Absolute) {
                    return {instruction_str[di.i], (unsigned short)(bus.unsafe_read_prg( address + 1) | (bus.unsafe_read_prg( address + 2) << 8)), 0, false,
                            false, false};
                } else { // Indirect
                    unsigned short arg_addr = (bus.unsafe_read_prg( address + 1) | (bus.unsafe_read_prg( address + 2) << 8));
                    unsigned char l = bus.unsafe_read_prg( arg_addr);
                    unsigned char h = bus.unsafe_read_prg( (arg_addr & 0xff) == 0xff ? (arg_addr & 0xff00) : arg_addr + 1);
                    return {instruction_str[di.i],(unsigned short)(l | (h << 8)), 0, false , false, false};
                }
            }

            default:
                { return {instruction_str[di.i], 0, 0, false, true, false}; }
        }
    }
};
