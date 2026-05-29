#include <iostream>
#include <iostream>
#include <fstream>
#include <cstring>
#include "Files.hpp"
#include "Dwarf.hpp"
#include "Macho.hpp"


int main(int argc, char** argv){
    if (argc != 2) std::abort();
    auto f = read_file(argv[1]);
    print_macho_header(f);
    print_dwarf_cu_header(f, find_section(f, "__DWARF", "__debug_info"));
    print_dwarf_debug_line_header(f, find_section(f, "__DWARF", "__debug_line"));
}
