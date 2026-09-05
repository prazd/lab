#ifndef SMD_CPU_HPP
#define SMD_CPU_HPP
#include <queue>
#include "BUS.hpp"
#include "Simple.hpp"
struct MC68K {
    // SPR
    unsigned pc{};
    // User stack pointer
    unsigned usp{};
    // Supervisor stack pointer
    unsigned ssp{};
    // Vector base register
    unsigned vbr{};
    // Alternate function code registers
    unsigned sfc{};
    unsigned dfc{};

    struct PrefetchQ {
        unsigned prefetch_addr{};
        SimpleDequeT<unsigned short, 2> q;
        unsigned char prefetch_q_ticks = 4;
        inline void prefetch(BUS& bus){
            if (q.size() != 2) {
                --prefetch_q_ticks;
                    if (!prefetch_q_ticks) {
                    unsigned short d = 
                        (bus.mc68kmem[prefetch_addr + 1] << 8) | bus.mc68kmem[prefetch_addr];
                    q.push_back(d);
                    prefetch_q_ticks = 4;
                    prefetch_addr+=2;
                }
            }
        }
        inline unsigned short get(){
            auto res = q.front();
            q.pop_front();
            return res;
        }
        inline void set_address(unsigned addr) {
            prefetch_addr = addr;
        }
    } prefetch_q;

    
    // Status register
    unsigned short sreg{};
    enum SREG: unsigned short {
        Carry=0x0,Overflow=0x2,Zero=0x4,Negative=0x8,Extend=0x10,
        Interrupt0=0x100,Interrupt1=0x200,Interrupt2=0x400,
        State0=0x1000,State1=0x2000,TraceMode0=0x4000,TraceMode1=0x8000
    };
    inline void set_sreg(SREG s, bool val){
        sreg &= ~(1 << s);
        sreg |= (val << s);
    }
    inline bool get_srge(SREG s){
        return sreg & s;
    }

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

    struct AddressModeVal {
        unsigned* ptr{};
        AddressMode type{};
    };

    AddressMode to_adress_mode(unsigned char);

    enum Instruction {
        ABCD,ADD,ADDA,ADDI,ADDQ,ADDX,AND,ANDI,ASL,ASR,
        BCC,BCHG,BCLR,BRA,BSET,BSR,BTST,
        CHK,CLR,CMP,CMPA,CMPI,CMPM,
        DBcc, DIVS, DIVSL,DIVU,DIVUL,
        EOR,EORI,EORItoCCR,EXG,EXT,
        ILLEGAL,
        JMP,JSR,
        LEA,LINKw,LINKl,LSL,LSR,
        MOVE,MOVEA,MOVEtoCCR,MOVEfromSR,MOVEM,MOVEP,MOVEQ,MULS,MULU,
        NBCD,NEG,NEGX,NOP,NOT,
        OR,ORI,ORItoCCR,
        PEA,
        ROL,ROR,ROXL,ROXR,RTM,RTR,RTS,
        SBCD,Scc,SUB,SUBA,SUBI,SUBQ,SUBX,SWAP,
        TAS,TRAP,TRAPV,TST,
        UNLK,
    };

    enum RegType {
        Dn,An
    };
    enum BusReadDest: unsigned char {
        BRDOperandA1,
        BRDOperandA2,
        BRDOperandB1,
        BRDOperandB2,
        BRDSignExtend,
    };

    enum InternalOp: unsigned char {
        FetchDec,
        BusReadReq,
        BusReadWait,
        BusWriteReq,
        BusWriteWait,

        // Effective address
        Nop,
        RegDec,
        RegInc,
        ExecAdd,
    };


    RegType ea_reg_type;
    unsigned char ea_reg_n{};

    void ea_fill(unsigned char);

    void fill_read_req(unsigned addr, BusReadDest, OperandSize);

    unsigned sign_extend_to_l(unsigned short);

    unsigned char dest_reg{};
    unsigned char source_reg{};

    struct IntOperationReq{
        unsigned address{};
        BusReadDest brd{};
        InternalOp iop{};
        unsigned char req_count{};
    };

    SimpleDequeT<IntOperationReq, 20> iops_q;

    struct DecodedInfo {
        Instruction i;
        bool address_reg{};
        unsigned char rx{};
        unsigned char ry{};
        unsigned char d1{};
        unsigned char d2{};
        unsigned char d3{};
        unsigned char d4{};
        unsigned char operand_sz{};
        unsigned mask{};
        unsigned sign_bit{};
        unsigned i_address{};
        AddressMode addr_mode{};
        unsigned char effective_address{};

        BUS::Mode read_mode{};

        OperandSize operand_a_sz{};
        OperandSize operand_b_sz{};
        bool f_a_read = true;
        bool f_b_read = true;
        unsigned operand_a_address{};
        unsigned operand_b_address{};
    } decoded_info;

    unsigned short operand_a_w1{};
    unsigned short operand_a_w2{};
    unsigned short operand_b_w1{};
    unsigned short operand_b_w2{};
    unsigned operand_a_l{};
    unsigned operand_b_l{};
    inline void operands_a_to_long(){operand_a_l = (operand_a_w2 << 16) | operand_a_w1;}
    inline void operands_b_to_long(){operand_b_l = (operand_b_w2 << 16) | operand_b_w1;}

    AddressMode am{};
    unsigned effective_address_value{};

    unsigned bus_address{};
    unsigned short bus_data{};
    BUS::Mode bus_mode{};

    unsigned& read_operand(unsigned char effective_address, BUS& bus);
    DecodedInfo decode(unsigned short);

    void exec(DecodedInfo di, BUS& bus);
    void tick(BUS& bus);
};
#endif