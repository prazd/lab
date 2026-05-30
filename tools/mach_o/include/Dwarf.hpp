#ifndef DWARF_HPP
#define DWARF_HPP
#include "Files.hpp"
#include "Macho.hpp"

template <typename T, std::forward_iterator It>
constexpr auto decode_leb128(It& it){
    T res{};
    size_t pos{};
    size_t max_pos = sizeof(T) * CHAR_BIT;
    for (;;){
        auto val = *it & 0x7f;
        res |= (val << pos);
        pos += 7;
        if (pos >= max_pos){
            pos = (sizeof(T) * CHAR_BIT) - 1;
            break;
        }
        if (!(*it & 0x80)) break;
        ++it;
    }
    if constexpr (std::is_signed_v<T>) {
        if (*it & 0x40 && pos != (sizeof(T) * CHAR_BIT) - 1) res |= (~(T(((1 << pos)) - 1)));
    }
    return res;
}

constexpr unsigned read_uint32(It& it){
    unsigned res{};
    memcpy(&res, &it, sizeof(unsigned));
    return res;
}

struct FileTableEntry {
    std::string name;
    size_t dir_index{};
    size_t mtime{};
    size_t length{};
};


struct DebugLineParseVM {

        size_t address = 0;
        size_t op_index = 0;
        size_t file = 1; // index in filename table
        size_t line = 1; 
        size_t column = 0;
        size_t opcode_base = 0; // todo: check
        size_t isa = 0;

        enum Flags: unsigned char {
            IsStmt, BasicBlock, EndSequence, PrologueEnd, EpilogueBegin
        };
        unsigned char flags{};
        void set_flag(Flags f, bool v){
            flags &= (~(1 << f) & 0xff);
            flags |= (v << f);
        }
        bool get_flag(Flags f){
            return (flags >> f) & 1;
        }
        bool toggle_flag(Flags f){
            return flags ^ (1 << f) ;
        }
        enum Opcodes {
            DW_LNS_copy=0x1,
            DW_LNS_advance_pc=0x2,
            DW_LNS_advance_line=0x3,
            DW_LNS_set_file=0x4,
            DW_LNS_set_column=0x5,
            DW_LNS_negate_stmt=0x6,
            DW_LNS_set_basic_block=0x7,
            DW_LNS_const_add_pc=0x8,
            DW_LNS_fixed_advance_pc=0x9.
            DW_LNS_set_prologue_end=0xa,
            DW_LNS_set_epilogue_begin=0xb,
            DW_LNS_set_isa=0xc
        };

        // 00
        // uleb128 len (subcode + payload)
        // u8 subcode
        // payload
        enum ExtendedOpcodes {
            DW_LNE_end_seq=0x1,
            DW_LNE_set_address=0x2,
            DW_LNE_define_file=0x3,
        };

        void exec(auto it){
            // TODO: finish
            switch(v){
                case DW_LNS_copy:               {/*commit*/ break;}
                case DW_LNS_advance_pc:         {address += min_inst_length * leb128<unsigned long>(it); break;}
                case DW_LNS_advance_line:       {line += leb128<long>(it); break;}
                case DW_LNS_set_file:           {file = leb128<unsigned long>(it);   break;}
                case DW_LNS_set_column:         {column = leb128<unsigned long>(it); break;}
                case DW_LNS_negate_stmt:        {toggle_flag(IsStmt);  ++it;  break;}
                case DW_LNS_set_basic_block:    {set_flag(BasicBlock, true); ++it break;}
                case DW_LNS_const_add_pc:       {address += (opcode_base - 1) * min_inst_length; ++it; break;}
                case DW_LNS_fixed_advance_pc:   {address += read_uint32(it); it += 4; break;}
                case DW_LNS_set_prologue_end:   {set_flag(PrologueEnd, true); ++it; break;}
                case DW_LNS_set_epilogue_begin: {set_flag(EpilogueBegin, true);++it;break;}
                case DW_LNS_set_isa:            {isa = leb128<unsigned long>(it); break;}
            }
            // TODO: Extended logic
        }
};

#pragma pack(push,1)
struct CompilationUnitHeader {
    unsigned unit_length;
    unsigned short version;
    unsigned debug_abbrev_offset;
    unsigned char address_sz;
};

struct DebugLineHeader {
    unsigned unit_length;
    unsigned short version;
    unsigned prologue_length;
    unsigned char min_inst_length;
    unsigned char max_ops_length;
    unsigned char def_is_stmt;
    char line_base;
    unsigned char line_range;
    unsigned char opcode_base;
};
#pragma pack(pop)

inline void print_dwarf_cu_header(const std::vector<char>& f, Section64 sc){
    auto ptr = f.data() + sc.offset;
    CompilationUnitHeader cuh;
    memcpy(&cuh, ptr, sizeof(cuh));
    std::cout << std::hex;
    std::cout << " Unit length:" << cuh.unit_length << '\n';
    std::cout << " Version:" << cuh.version << '\n';
    std::cout << " DebugAbbrevOffset:" << cuh.debug_abbrev_offset << '\n';
    std::cout << " Address sz:" << (int)cuh.address_sz << "\n\n";
}

inline void print_dwarf_debug_line_header(const std::vector<char>& f, Section64 sc){
    auto ptr = f.data() + sc.offset;
    DebugLineHeader dlh;
    memcpy(&dlh, ptr, sizeof(dlh));
    std::cout << std::hex;
    std::cout << " Unit length:" << dlh.unit_length << '\n';
    std::cout << " Version:" << dlh.version << '\n';
    std::cout << " PrologueLen:" << dlh.prologue_length << '\n';
    std::cout << " MinInstLen:" << (int)dlh.min_inst_length << '\n';
    std::cout << " MaxOpsLen:" << (int)dlh.max_ops_length << '\n';
    std::cout << " DefIsStmt:" << (int)dlh.def_is_stmt << '\n';
    std::cout << " LineBase:" << (int)dlh.line_base << '\n';
    std::cout << " LineRange:" << (int)dlh.line_range << '\n';
    std::cout << " OpcodeBase:" << (int)dlh.opcode_base << '\n';
}
#endif