#ifndef SMD_CPU_HPP
#define SMD_CPU_HPP
#include <queue>
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
        CC_CC,CC_CS,CC_EQ,
        CC_GE,CC_GT,CC_HI,CC_LE,
        CC_LS,CC_LT,CC_MI,CC_NE,CC_PL,CC_VC,CC_VS
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

    enum OperandSize: unsigned char {BYTE=0x1,WORD=0x2,LONG=0x4};

    OperandSize opsz = BYTE;

    enum AddressMode: unsigned char{
        DataRegisterDirectMode,
        AddressRegisterDirectMode,
        AddressRegisterIndirectMode,
        AddressRegisterIndirectPostIncMode,
        AddressRegisterIndirectPreDecMode,
        AddressRegisterIndirectDisplacementMode,
        AddressRegisterIndirectIndexMode,
        ProgramCounterInderectDisplacementMode,
        ProgramCounterInderectIndexMode,
        AbsoluteShort,
        AbsoluteLong,
        Immediate,
        Unknown,
    };

    static AddressMode to_address_mode(unsigned char, bool);

    enum Instruction {
        ABCD,ADD,ADDA,ADDI,ADDQ,ADDX,AND,ANDI,ASL,ASR,ANDItoCCR,
        BCC,BCHG,BCLR,BRA,BSET,BSR,BTST,
        CHK,CLR,CMP,CMPA,CMPI,CMPM,
        DBcc, DIVS,
//        TODO: check
//        DIVSL ,
//        DIVUL
        DIVU,
        EOR,EORI,EORItoCCR,EXG,EXT,
        ILLEGAL,

        JMP,JSR,
        LEA,LINK,LSL,LSR,

        MOVE,MOVEA,MOVEtoCCR,MOVEfromSR,

        MOVEM,MOVEP,MOVEQ,MULS,MULU,
        NBCD,NEG,NEGX,NOP,NOT,
        OR,ORI,ORItoCCR,
        PEA,
        ROL,ROR,ROXL,ROXR,RTR,RTS,

        SBCD,Scc,SUB,SUBA,SUBI,SUBQ,SUBX,

        SWAP,TAS,TRAP,TRAPV,TST,
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
        FetchDecode,
        ReadEA,
        BusReadReq,
        BusReadWait,
        BusWriteReq,
        BusWriteWait,

        // Effective address
        Nop,
    };

// TODO:

    unsigned ea_address{};
    AddressMode ea_mode;
    RegType ea_reg_type;
    unsigned char ea_reg_n{};

    void ea_fill(unsigned char);
    void ea_write(unsigned char, unsigned val);
    unsigned to_ea_address(unsigned char);

    void fill_bus_read_req(unsigned, BusReadDest, OperandSize);
    void fill_bus_read_req_reg(unsigned, BusReadDest, OperandSize, unsigned char);
    void fill_bus_write_req(unsigned, unsigned, OperandSize);

    template <std::integral T>
    requires(std::is_same_v<T, unsigned char> || 
             std::is_same_v<T, unsigned short>)
    unsigned sign_extend_to_l(T v) {
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
        unsigned char reg_n{};
        OperandSize opsz{};
    };

    SimpleDequeT<IntOperationReq, 20> iops_q;

    struct DecodedInfo {
        Instruction i;
        unsigned char rx{};
        unsigned char ry{};
        unsigned char d1_b{};
        unsigned char d2_b{};
        unsigned char d3_b{};
        unsigned d_l{};
        unsigned prefetched{};
        unsigned char effective_address{};
        bool read_ea{};
        unsigned immediate{}; // TODO: update
        unsigned char opmode{};
        OperandSize opsz{};
        unsigned sign_bit{};
        unsigned mask{};

        bool read_displacement{};
        bool read_displacement_by_value{};
        bool read_predec_args{}; // todo: check
        bool read_postinc_args{}; // todo: check
        bool read_prefetch{};
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
        switch (opsz) {
            case BYTE:
            case WORD: {
                fill_bus_read_req(aregs[di.rx]-=opsz,BRDOperandA1, opsz);
                fill_bus_read_req(aregs[di.ry]-=opsz,BRDOperandB1, opsz);
                break;
            }
            case LONG: {
                fill_bus_read_req(aregs[di.rx]-=2,BRDOperandA1, WORD);
                fill_bus_read_req(aregs[di.rx]-=2,BRDOperandA2, WORD);
                fill_bus_read_req(aregs[di.ry]-=2,BRDOperandB1, WORD);
                fill_bus_read_req(aregs[di.ry]-=2,BRDOperandB2, WORD);
                break;
            }
        }
    }


    // TODO: check
    inline void fill_post_inc_args(){
        switch (opsz) {
            case BYTE:
            case WORD: {
                fill_bus_read_req(std::exchange(aregs[di.rx], aregs[di.rx]+opsz), BRDOperandA1, opsz);
                fill_bus_read_req(std::exchange(aregs[di.ry], aregs[di.ry]+opsz), BRDOperandA1, opsz);
                break;
            }
            case LONG: {
                fill_bus_read_req(std::exchange(aregs[di.rx], aregs[di.rx]+2), BRDOperandA1, WORD);
                fill_bus_read_req(std::exchange(aregs[di.rx], aregs[di.rx]+2), BRDOperandA2, WORD);
                fill_bus_read_req(std::exchange(aregs[di.ry], aregs[di.ry]+2), BRDOperandB1, WORD);
                fill_bus_read_req(std::exchange(aregs[di.ry], aregs[di.ry]+2), BRDOperandB2, WORD);
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
    unsigned short res_w1{};
    unsigned short res_w2{};

    unsigned operand_a_l{};
    unsigned operand_b_l{};

    inline void operands_a_to_long(){operand_a_l = (operand_a_w2 << 16) | operand_a_w1;}
    inline void operands_b_to_long(){operand_b_l = (operand_b_w2 << 16) | operand_b_w1;}

    AddressMode am{};
    unsigned effective_address_value{};

    unsigned bus_address{};
    unsigned short bus_data{};
    BUS::Mode bus_mode{};

    static DecodedInfo decode(unsigned short);

    void exec(DecodedInfo di, BUS& bus);
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
    inline void reset_prefetch_q(){
        prefetch_q.clear();
        fill_prefetch_req();
        fill_prefetch_req();
    }

    inline void jump(unsigned addr){
        pc = addr;
        reset_prefetch_q();
    }

    unsigned char exec_cycles(const DecodedInfo& di);
    unsigned char ea_fetch_cycles(AddressMode am, OperandSize sz);

    void fill_exec(Instruction);
    void set_ea_dst();
};
#endif