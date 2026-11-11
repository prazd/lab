#ifndef SMD_CPU_HPP
#define SMD_CPU_HPP
#include <queue>
#include <algorithm>
#include "BUS.hpp"
#include "Simple.hpp"
struct M68K {
    // SPR
    unsigned pc{};
    // User stack pointer
    unsigned usp{};
    // Supervisor stack pointer
    unsigned ssp{};

    enum ConditionCode: unsigned char {
        CC_BRA=0x0, CC_BSR=0x1,
        CC_CC=0x4,CC_CS=0x5,CC_EQ=0x7,
        CC_GE=0xC,CC_GT=0xe,CC_HI=0x2,CC_LE=0xf,
        CC_LS=0x3,CC_LT=0xd,CC_MI=0xa,CC_NE=0x6,CC_PL=0xb,CC_VC=0x8,CC_VS=0x9
    };
    static constexpr unsigned char cond_code_list[] = {
            CC_CC,CC_CS,CC_EQ,
            CC_GE,CC_GT,CC_HI,CC_LE,
            CC_LS,CC_LT,CC_MI,CC_NE,CC_PL,CC_VC, CC_VS
    };

    // TODO: exception handing
    enum ExceptionVector {
        EV_ResetInitialSSP=0x0,
        EV_ResetInitialPC=0x4,
        EV_AccessFault=0x8,
        EV_AddressErr=0xc,
        EV_IllegalInstruction=0x10,
        EV_ZeroDivide=0x14,
        EV_CheckInstruction=0x18,
        EV_TRAPV=0x1c,
        EV_PrivilegeViolation=0x20,
        EV_Trace=0x24,
    };

    bool is_cond(ConditionCode cc);

    // Status register
    unsigned short sreg{};
    enum SREG: unsigned short {
        Carry=0x0,Overflow=0x2,Zero=0x4,Negative=0x8,Extend=0x10,
        Interrupt0=0x100,Interrupt1=0x200,Interrupt2=0x400,
        SupervisorMode=0x2000,TraceMode0=0x4000,TraceMode1=0x8000
    };
    inline void set_sreg(SREG s, bool val){
        sreg &= ~(1 << s);
        sreg |= (val << s);
    }
    inline bool get_sreg(SREG s){
        return sreg & s;
    }
    #define SP (!get_sreg(SupervisorMode) ? usp : ssp)

    // GPR
    // Data registers
    enum DREG: unsigned char {D0,D1,D2,D3,D4,D5,D6,D7};
    unsigned dregs[8]{};
    
    // Address registers
    enum AREG: unsigned char {A0,A1,A2,A3,A4,A5,A6,A7};
    unsigned aregs[8]{};

    enum OperandSize: unsigned char {Unsized=0x0,BYTE=0x1,WORD=0x2,LONG=0x4};

    OperandSize opsz = BYTE;
    size_t exec_cycles = 0;

    enum AddressMode: unsigned char{
        DataRegisterDirectMode,
        AddressRegisterDirectMode,
        AddressRegisterIndirectMode,
        AddressRegisterIndirectPostIncMode,
        AddressRegisterIndirectPreDecMode,
        AddressRegisterIndirectDisplacementMode,
        AddressRegisterIndirectIndexMode,
        ProgramCounterIndirectDisplacementMode,
        ProgramCounterIndirectIndexMode,
        AbsoluteShort,
        AbsoluteLong,
        Immediate,
        Unknown,
    };

    static AddressMode to_address_mode(unsigned char, bool);

    enum Instruction {
        ABCD,ADD,ADDA,ADDI,ADDQ,ADDX,AND,ANDI,ASL,ASR,ANDItoCCR,
        Bcc,BCHG,BCLR,BRA,BSET,BSR,BTST,
        CHK,CLR,CMP,CMPA,CMPI,CMPM,
        DBcc, DIVS,
        DIVU,
        EOR,EORI,EORItoCCR,EXG,EXT,
        ILLEGAL,

        JMP,JSR,
        LEA,LINK,LSL,LSR,

        MOVE,MOVEA,MOVEtoCCR,MOVEfromSR,MOVEM,MOVEP,MOVEQ,MULS,MULU,
        NBCD,NEG,NEGX,NOP,NOT,
        OR,ORI,ORItoCCR,
        PEA,
        ROL,ROR,ROXL,ROXR,RTR,RTS,

        SBCD,Scc,SUB,SUBA,SUBI,SUBQ,SUBX,SWAP,
        TAS,TRAP,TRAPV,TST,
        UNLK,

//  Supervisor
        ANDItoSR,
        EORItoSR,
        MOVEtoSR,
        MOVEUSP,
        ORItoSR,
        RESET,
        RTE,
        STOP,
    };

    static constexpr std::string_view instructions_str[] = {
            //                        (ADDA) for test
            "ABCD","ADD","ADD","ADDI","ADDQ","ADDX","AND","ANDI","ASL","ASR","ANDI",
            "Bcc","BCHG","BCLR","BRA","BSET","BSR","BTST",
            "CHK","CLR","CMP","CMPA","CMPI","CMPM",
            "DBcc", "DIVS", "DIVU",
            "EOR","EORI","EORI","EXG","EXT",
            "ILLEGAL",
            "JMP", "JSR",
            "LEA", "LINK", "LSL", "LSR",
            //               (MOVEA) for test
            "MOVE", "MOVE","MOVE", "MOVE","MOVEM","MOVEP","MOVEQ","MULS","MULU",
            "NBCD","NEG","NEGX","NOP","NOT",
            "OR","ORI","ORI",
            "PEA",

            "ROL","ROR","ROXL","ROXR","RTR","RTS",
            "SBCD","Scc","SUB","SUBA","SUBI","SUBQ","SUBX","SWAP",
            "TAS","TRAP","TRAPV","TST",
            "UNLK",

             "ANDI", // to SR
             "EORI", // to SR
          "MOVE", // to SR
          "MOVE", // USP
          "ORI", // to SR,

            "RESET",
            "RTE",
              "STOP",
    };

    static constexpr std::string_view bcc_cond_to_str[] = {
            "BRA","BSR","BHI","BLS","BCC","BCS","BNE",
            "BEQ","BVC","BVS","BPL","BMI","BGE","BLT","BGT","BLE"
    };
    static constexpr std::string_view dbcc_cond_to_str[] = {
            "DBT","DBF","DBHI","DBLS","DBCC","DBCS","DBNE",
            "DBEQ","DBVC","DBVS","DBPL","DBMI","DBGE","DBLT","DBGT","DBLE"
    };

    static constexpr Instruction unsized_instructions[] = {
            STOP,RTE,RESET,UNLK,TRAP,TRAPV,RTR,RTS,
            NOP,ILLEGAL, JSR, JMP,
    };

    static constexpr char opsz_c[] = {
           '\0','B','W','\0','L',
    };

    enum RegType {
        Dn,An
    };

    enum BusReadDest: unsigned char {
        BRDOperandA1,
        BRDOperandA2,
        BRDOperandB1,
        BRDOperandB2,

        BRDPrefetchQ,
        // TODO: check L/H
        BRDDataRegisterL,
        BRDDataRegisterH,
        BRDAddressRegisterL,
        BRDAddressRegisterH,

        BRDDataRegisterByteL,
        BRDDataRegisterByteH,

        BRD_PC_L,
        BRD_PC_H,
        BRD_SSP_L,
        BRD_SSP_H,
        BRD_SREG,

    };

    enum InternalOp: unsigned char {
        ReadEA,
        BusReadReq,
        BusReadWait,
        BusWriteReq,
        BusWriteWait,
        BusWriteBackReq,
        INOP,
        Execute,
    };

    enum OperandType: unsigned char {
        OT_EMPTY,
        OT_EA,
        OT_IMM,
        OT_DREG,
        OT_AREG,
        OT_CCR,
        OT_SR,
        OT_USP,
        OT_LABEL, // TODO: check
        OT_LIST, // TODO: check
    };

    unsigned ea_src_addr{};
    unsigned ea_dst_addr{};
    AddressMode ea_mode;
    RegType ea_reg_type;
    unsigned char ea_reg_n{};

    unsigned ea_address_calc(unsigned char);
    void ea_fill(unsigned char);
    void ea_write(unsigned char, unsigned val);
    unsigned to_ea_address(unsigned char);

    void fill_bus_read_req(unsigned, BusReadDest, OperandSize,unsigned char = 0);
    void fill_bus_read_req_front(unsigned, BusReadDest, OperandSize,unsigned char = 0);
    void fill_bus_write_req(unsigned, unsigned, OperandSize);
    void fill_write_back_req(OperandSize);

    template <std::integral T>
    requires(std::is_same_v<T, unsigned char> || 
             std::is_same_v<T, unsigned short>)
    static unsigned sign_extend_to_l(T v) {
        bool is_word = std::is_same_v<T, unsigned short>;
        return v & (is_word ? 0x8000 : 0x80) ? 
            (is_word ? (0xffff0000 | v) : (0xffffff00 | v)) : v;
    };

    struct IntOperationReq{
        unsigned address{};
        InternalOp iop{};
        BusReadDest brd{};
        unsigned short bus_data{};
        BUS::Mode bus_mode{};
        unsigned char n{};
        OperandSize opsz{};
        bool parallel_run{};
        Instruction i{};
    };

    SimpleDequeT<IntOperationReq, 20> iops_q;

    inline void iop_push_nops(size_t n){
        for (size_t i = 0; i < n; ++i)
            iops_q.push_back({.iop = INOP});
    }

    struct DecodedInfo {
        Instruction i;
        unsigned char r_src{};
        unsigned char r_dst{};

        unsigned char d1_b{};

        unsigned d_l{};
        unsigned prefetched{};
        unsigned char ea_src{};
        unsigned char ea_dst{};
        bool read_ea{};
        bool calc_ea{};
        unsigned char opmode{};
        OperandSize opsz{};
        unsigned sign_bit{};
        unsigned mask{};

        bool read_displacement{};
        bool read_displacement_by_value{};
        bool read_predec_args{}; // todo: check
        bool read_postinc_args{}; // todo: check
        bool read_prefetch{};
        unsigned char i_sz{};
        OperandType src_type{};
        OperandType dst_type{};
        bool unknown = false;
    } di;

    inline void read_displacement() {
        // TODO: check
        di.d_l = read_prefetch_q();
        if (di.opsz == BYTE){
            di.d_l &= 0xff;
        } else if (di.opsz == LONG){
            di.d_l <<= 16;
            di.d_l |= read_prefetch_q();
        }
    }

    inline void fill_predec_args(){
        // TODO: check
        switch (opsz) {
            case BYTE:
            case WORD: {
                fill_bus_read_req(aregs[di.r_dst]-=opsz, BRDOperandA1, opsz);
                fill_bus_read_req(aregs[di.r_src]-=opsz, BRDOperandA2, opsz); // TODO: check
                break;
            }
            case LONG: {
                fill_bus_read_req(aregs[di.r_dst]-=2, BRDOperandA1, WORD);
                fill_bus_read_req(aregs[di.r_dst]-=2, BRDOperandA2, WORD);
                fill_bus_read_req(aregs[di.r_src]-=2, BRDOperandB1, WORD);
                fill_bus_read_req(aregs[di.r_src]-=2, BRDOperandB2, WORD);
                break;
            }
        }
    }


    // TODO: check
    inline void fill_post_inc_args(){
        switch (opsz) {
            case BYTE:
            case WORD: {
                fill_bus_read_req(std::exchange(aregs[di.r_dst], aregs[di.r_dst] + opsz), BRDOperandA1, opsz);
                fill_bus_read_req(std::exchange(aregs[di.r_src], aregs[di.r_src] + opsz), BRDOperandA1, opsz);
                break;
            }
            case LONG: {
                fill_bus_read_req(std::exchange(aregs[di.r_dst], aregs[di.r_dst] + 2), BRDOperandA1, WORD);
                fill_bus_read_req(std::exchange(aregs[di.r_dst], aregs[di.r_dst] + 2), BRDOperandA2, WORD);
                fill_bus_read_req(std::exchange(aregs[di.r_src], aregs[di.r_src] + 2), BRDOperandB1, WORD);
                fill_bus_read_req(std::exchange(aregs[di.r_src], aregs[di.r_src] + 2), BRDOperandB2, WORD);
                break;
            }
        }
    }

    inline void read_displacement_by_value() {
        di.d_l = read_prefetch_q();
        if (di.d_l & 0xff) {
            di.d_l &= 0xff;
            opsz = BYTE;
        }else if (di.d_l) {
            opsz = WORD;
        } else if (!di.d_l) {
            di.d_l |= (read_prefetch_q() << 16);
            opsz = LONG;
        }
    }


    inline void update_mask_n_sign_bit(){
        switch (opsz) {
            case BYTE: di.sign_bit = 0x80; di.mask = 0x000000ff; break;
            case WORD: di.sign_bit = 0x8000; di.mask = 0x0000ffff; break;
            case LONG: di.sign_bit = 0x80000000; di.mask = 0xffffffff; break;
        }
    };

    static inline unsigned char ea_i_sz(unsigned char val, size_t opsz, bool mode = true){
        switch (to_address_mode(val, mode)) {
            case AddressRegisterIndirectDisplacementMode:
            case AddressRegisterIndirectIndexMode:
            case ProgramCounterIndirectDisplacementMode:
            case ProgramCounterIndirectIndexMode:
            case AbsoluteShort: return 2;
            case AbsoluteLong: return 4;
            case Immediate: return (opsz == LONG ? 4 : 2); // TODO: check
            default: return 0;
        }
    }

    unsigned short absolute_l_w1{};

    enum Dest{
        DSTAn,DSTDn,DSTMem,
    } dst;
    unsigned dst_addr{};
    unsigned dst_value{};
    unsigned char dst_reg_n{};
    void write_dst();

    unsigned short operand_a_w1{};
    unsigned short operand_a_w2{};
    unsigned short operand_b_w1{};
    unsigned short operand_b_w2{};

    unsigned operand_a_l{};
    unsigned operand_b_l{};

    inline void operands_a_to_long(){operand_a_l = (operand_a_w2 << 16) | operand_a_w1;}
    inline void operands_b_to_long(){operand_b_l = (operand_b_w2 << 16) | operand_b_w1;}

    AddressMode am{};

    static DecodedInfo decode(unsigned short);

    void exec(const DecodedInfo& di, BUS& bus);
    void tick(BUS& bus);

    unsigned prefetch_addr{};
    SimpleDequeT<unsigned short, 2> prefetch_q{};
    inline unsigned short read_n_fill_prefetch_q(){
        auto v = prefetch_q.front();
        prefetch_q.pop_front();
        fill_prefetch_req();
        return v;
    }
    inline unsigned short read_prefetch_q(){
        auto v = prefetch_q.front();
        prefetch_q.pop_front();
        return v;
    }

    inline void fill_prefetch_req(){
        fill_bus_read_req(prefetch_addr, BRDPrefetchQ, WORD);
        prefetch_addr += 2;
    }
    inline void fill_prefetch_req_front(){
        fill_bus_read_req_front(prefetch_addr, BRDPrefetchQ, WORD);
        prefetch_addr += 2;
    }
    inline void reset_prefetch_q(){
        prefetch_q.clear();
        prefetch_addr = pc;
        fill_prefetch_req();
        fill_prefetch_req();
    }

    inline void jump(unsigned addr){
        pc = addr;
        reset_prefetch_q();
    }

    unsigned char calc_exec_cycles(const DecodedInfo& di);
    unsigned char ea_fetch_cycles(AddressMode am, OperandSize sz);

    void fill_exec(Instruction);
    void set_ea_dst();
    
    template <bool mr = true>
    static std::pair<SmallString<128>,size_t> ea_to_str(unsigned char ea_val, OperandSize opsz, const unsigned char* mem) {
        auto am = to_address_mode(ea_val, mr);
        // TODO: check
        unsigned char reg_n = mr ?  ea_val & 0x7 : ((ea_val >> 3) & 0x7);
        switch (am) {
            case DataRegisterDirectMode:      return {str_num_template<128>("d&", reg_n),0};
            case AddressRegisterDirectMode:   return {str_num_template<128>("a&", reg_n),0};
            case AddressRegisterIndirectMode: return {str_num_template<128>("(a&)", reg_n), 0};
            case AddressRegisterIndirectPostIncMode: return {str_num_template<128>("(a&)+", reg_n),0};
            case AddressRegisterIndirectPreDecMode:  return {str_num_template<128>("-(a&)", reg_n), 0};
            case AddressRegisterIndirectDisplacementMode:
                return {
                        str_num_template<128,false>("(&,",   sign_extend_to_l((unsigned short)read_mem(WORD, mem)), true) +
                        str_num_template<128>("a&)", reg_n),opsz == LONG ? LONG : WORD
                };
            case AddressRegisterIndirectIndexMode:{
                auto val = (unsigned short)read_mem(WORD, mem);
                OperandSize sz = (val & 0x800) ? LONG : WORD;
                return {str_num_template<128,false>("(&,", sign_extend_to_l((unsigned char)(val & 0xff)), true) + str_num_template<128>("a&,", reg_n) +
                        ((val & 0x8000) ? str_num_template<128>("a&)", ((*mem) >> 12) & 0x7) :
                         str_num_template<128>("d&.", ((*mem) >> 12) & 0x7)) + str_char_template<128>("&)", {opsz_c[sz]}), 2};
            }
            case ProgramCounterIndirectDisplacementMode: {
                return {
                        str_num_template<128,false>("(&,",   sign_extend_to_l((unsigned short)read_mem(WORD, mem)), true) +
                        str_num_template<128>("pc)", reg_n),opsz == LONG ? LONG : WORD
                };
            }
            case ProgramCounterIndirectIndexMode: {
                auto val = (unsigned short)read_mem(WORD, mem);
                OperandSize sz = (val & 0x800) ? LONG : WORD;
                return {str_num_template<128,false>("(&,", sign_extend_to_l((unsigned char)(val & 0xff)), true) + str_num_template<128>("pc,", 0) +
                        ((val & 0x8000) ? str_num_template<128>("a&.", ((*mem) >> 12) & 0x7) :
                         str_num_template<128>("d&.", ((*mem) >> 12) & 0x7)) + str_char_template<128>("&)", {opsz_c[sz]}), 2};
            }
            case AbsoluteShort:  return {str_num_template<128>("#&.W", read_mem(WORD, mem)), 2};
            case AbsoluteLong:  return {str_num_template<128>("#&.L", read_mem(LONG, mem)), 4};
            default:{

                return {str_num_template<128>("#&", read_mem(opsz, mem)), opsz == LONG ? 4 : 2};
            }

        }
    }

    static std::pair<SmallString<128>,size_t> read_operand(const M68K::DecodedInfo &di, unsigned pc, bool src, const unsigned char* mem, unsigned char* base);
    static unsigned read_mem(OperandSize sz, const unsigned char*);

    static constexpr auto get_opcode_str(const M68K::DecodedInfo& di){
        switch (di.i) {
            case Bcc: return bcc_cond_to_str[di.d1_b];
            case DBcc: return dbcc_cond_to_str[di.d1_b];
            default:
                return instructions_str[di.i];
        }
    }

    template <bool lowercase = true>
    static SmallString<128> disassemble(const M68K::DecodedInfo& di, unsigned pc, unsigned char* base) {
        SmallString<128> res{};
        std::string_view i_str = get_opcode_str(di);
        if constexpr (lowercase)
            for (auto c: i_str) res.push_lowercase(c);
        else
            for (auto c: i_str) res.push(c);

        if constexpr (lowercase)  {
            if (!std::any_of(std::begin(unsized_instructions), std::end(unsized_instructions), [i = di.i](auto& el){return el == i;})){
                res.push('.');
                res.push_lowercase(opsz_c[di.opsz]);
            }
        } else {
            if (!std::any_of(std::begin(unsized_instructions), std::end(unsized_instructions),
                             [i = di.i](auto &el) { return el == i; })) {
                res.push('.');
                res.push(opsz_c[di.opsz]);
            }
        }

        if (di.src_type != OT_EMPTY || di.dst_type != OT_EMPTY){
            res.push(' ');
            auto [str, step] = read_operand(di, pc, true, base + pc + 2, base);
            res += str;
            if (di.src_type != OT_EMPTY && di.dst_type != OT_EMPTY) res.push(',');
            res += read_operand(di, pc, false, base + (pc + 2 + step), base).first;
        }
        return res;
    }
};
#endif