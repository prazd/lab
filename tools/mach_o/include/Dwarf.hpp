#ifndef DWARF_HPP
#define DWARF_HPP
#include "Files.hpp"
#include "Macho.hpp"

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