#include <iostream>
#include <iostream>
#include <fstream>

template<typename T, size_t max_val>
struct MapNum {
    const T* const it_arr;
    size_t indexes[max_val + 1]{};
    constexpr MapNum(auto it_arr, auto it_end, auto it_i): it_arr(it_arr){
        for (size_t i = 0; it_arr != it_end; ++it_arr, ++i) {indexes[*(it_i + i)] = i;}
    }
    constexpr const auto& operator[](size_t index) const {
        return *(it_arr + (indexes[index]));
    }
};

struct MachOHeader {
    unsigned magic_number;
    unsigned cpu_type;
    unsigned cpu_subtype;
    unsigned file_type;
    unsigned number_of_load_commands;
    unsigned size_of_load_commands;
    unsigned flags;
    unsigned reserved;
};

enum CPUType {
    VAX=0x1, ROMP=0x2, NS32032=0x4,NS32332=0x5,MC680x0=0x6,x86=0x7,MIPS=0x8,NS32352=0x9,HP_PA=0xB,
    ARM=0xC,MC88000=0xD,SPARC=0xE,i860BE=0xF,i860LE=0x10,RS6000=0x11,POWERPC=0x12,RISCV=0x18, CPUTypeCount
};

constexpr size_t cpu_type_num[CPUTypeCount] = {
   VAX, ROMP, NS32032,NS32332,MC680x0,x86,MIPS,NS32352,HP_PA,
   ARM, MC88000,SPARC,i860BE,i860LE,RS6000,POWERPC,RISCV
};

constexpr std::string_view cpu_type_str[CPUTypeCount] = {
    "VAX", "ROMP", "NS32032","NS32332","MC680x0","x86","MIPS","NS32352","HP_PA",
    "ARM","MC88000","SPARC","i860BE","i860LE","RS6000","POWERPC","RISCV"
};

void print_header(const MachOHeader& header) {
    MapNum<std::string_view, 0x18> mn(cpu_type_str, cpu_type_str + CPUTypeCount, cpu_type_num);
    std::cout << "MagicNumber:" << header.magic_number << '\n';
    std::cout << "CpuType:" << mn[header.cpu_type & 0x1f] << '\n';
    std::cout << "FileType:" << header.file_type << '\n';
    std::cout << "NumberOfLoadCommands:" << header.number_of_load_commands << '\n';
    std::cout << "SizeOfLoadCommands:" << header.size_of_load_commands << '\n';
    std::cout << "Flags:" << header.flags << '\n';
    std::cout << "Reseved:" << header.reserved << '\n';
}

MachOHeader read_header(std::string_view file_path) {
    MachOHeader header;
    auto file = std::ifstream(file_path.data(), std::ios::binary);
    if (!file.is_open()) std::abort();
    file.read((char*)&header, sizeof(MachOHeader));
    return header;
}

int main(int argc, char** argv){
    if (argc != 2) std::abort();
    print_header(read_header(argv[1]));
}
