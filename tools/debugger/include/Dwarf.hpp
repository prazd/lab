#ifndef DWARF_HPP
#define DWARF_HPP
#include <cassert>
#include <climits>
#include <bit>
#include <array>
#include "Utils.hpp"
#include "Macho.hpp"
#define DEBUG 1


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
};

struct DebugLineHeaderV4 {
    unsigned unit_length;
    unsigned short version;
    unsigned prologue_length;
    unsigned char min_inst_length;
    unsigned char max_ops_length;
    unsigned char is_stmt;
    char line_base;
    unsigned char line_range;
    unsigned char opcode_base;
    inline void print(){
        PRINT(unit_length)
        PRINT(version)
        PRINT(prologue_length)
        PRINT_INT(min_inst_length)
        PRINT_INT(max_ops_length)
        PRINT_INT(is_stmt)
        PRINT_INT(line_base)
        PRINT_INT(line_range)
        PRINT_INT(opcode_base)
    }   
};

struct DebugLineHeaderV5 {
    unsigned unit_length;
    unsigned short version;
    unsigned char  address_size;
    unsigned char  segment_selector_size;
    unsigned prologue_length;
    unsigned char  min_inst_length;
    unsigned char  max_ops_length;
    unsigned char  is_stmt;
    char line_base;
    unsigned char  line_range;
    unsigned char  opcode_base;
    inline void print(){
        PRINT(unit_length)
        PRINT(version)
        PRINT_INT(address_size)
        PRINT_INT(segment_selector_size)
        PRINT(prologue_length)
        PRINT_INT(min_inst_length)
        PRINT_INT(max_ops_length)
        PRINT_INT(is_stmt)
        PRINT_INT(line_base)
        PRINT_INT(line_range)
        PRINT_INT(opcode_base)
    }
};
#pragma pack(pop)

enum ContentTypeV5 {
    DW_LNCT_path      = 0x1,
    DW_LNCT_dir_index = 0x2,
    DW_LNCT_timestamp = 0x3,
    DW_LNCT_size      = 0x4,
    DW_LNCT_MD5       = 0x5,
};

std::string_view to_sv(ContentTypeV5 ct) {
    switch(ct){
        case DW_LNCT_path      : return "DW_LNCT_path";      
        case DW_LNCT_dir_index : return "DW_LNCT_dir_index";
        case DW_LNCT_timestamp : return "DW_LNCT_timestamp"; 
        case DW_LNCT_size      : return "DW_LNCT_size";      
        case DW_LNCT_MD5       : return "DW_LNCT_MD5";       
        default:
            return "";
    }
}

enum FormV5 {
    DW_FORM_addr      = 0x1,
    DW_FORM_data1     = 0xb,
    DW_FORM_data2     = 0x5,
    DW_FORM_data4     = 0x6,
    DW_FORM_data8     = 0x7,
    DW_FORM_string    = 0x8,
    DW_FORM_udata     = 0xf,
    DW_FORM_strp      = 0xe,
    DW_FORM_line_strp = 0x1f,
};

std::string_view to_sv(FormV5 f){
    switch(f){
        case DW_FORM_addr     : return "DW_FORM_addr";    
        case DW_FORM_data1    : return "DW_FORM_data1";    
        case DW_FORM_data2    : return "DW_FORM_data2";    
        case DW_FORM_data4    : return "DW_FORM_data4";    
        case DW_FORM_data8    : return "DW_FORM_data8";    
        case DW_FORM_string   : return "DW_FORM_string";   
        case DW_FORM_udata    : return "DW_FORM_udata";    
        case DW_FORM_strp     : return "DW_FORM_strp";     
        case DW_FORM_line_strp: return "DW_FORM_line_strp";
        default:
            return "";
    }  
}

struct FieldV5 {
    ContentTypeV5 content_type;
    FormV5 form;
};

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
    ++it;
    return res;
}

struct DebugLineParseVM {
        struct FileTableEntry {
            std::string name;
            size_t dir_index{};
            size_t mtime{};
            size_t length{};
        };
        struct DebugLineHeaderLists {
            std::vector<unsigned char> standart_opcode_len;
            std::vector<std::string> include_directories;
            std::vector<FileTableEntry> file_entries;

            inline void print_standart_opcode_len(){
                std::cout << "standart opcode len:\n";
                for(auto el: standart_opcode_len)
                    std::cout << (int)el << '\n';
            }
            inline void print_include_directories(){
                std::cout << "include directories:\n";
                for(const auto& s: include_directories)
                    std::cout << s << '\n';
            }
            inline void print_file_entries(){
                std::cout << "file entries:\n";
                for(const auto& el: file_entries){
                    std::cout << "____\n";
                    std::cout << "Name:" << el.name << '\n';
                    std::cout << "DirIndex:" << el.dir_index << '\n';
                    std::cout << "Mtime:" << el.mtime << '\n';
                    std::cout << "Len:" << el.length << '\n';
                    std::cout << "____\n";
                }
            }
        } lists;

        const unsigned char* finish_address = nullptr;
        size_t opcode_base = 0; // todo: check
        size_t op_index = 0;

        bool is_stmt = false;
        unsigned char min_inst_length = 0;
        size_t isa = 0;

        struct State {
            size_t address = 0;
            size_t file = 1; // index in filename table
            size_t line = 1; 
            size_t column = 0;

            enum Flags: unsigned char {
                IsStmt, BasicBlock, EndSequence, PrologueEnd, EpilogueBegin
            };

            unsigned char flags{};
            inline void set_flag(Flags f, bool v){
                flags &= (~(1 << f) & 0xff);
                flags |= (v << f);
            }
            inline bool get_flag(Flags f){
                return (flags >> f) & 1;
            }
            inline bool toggle_flag(Flags f){
                return flags ^ (1 << f);
            }
            inline void print() const {
                PRINT(address)
                PRINT(file)
                PRINT(line)
                PRINT(column)
                PRINT((int)flags)
            }
        } state;
        std::vector<State> commited_states;

        unsigned char* it = 0;
       
        char line_base = 0;
        unsigned char line_range = 0;
   
        enum Opcodes {
            DW_LNS_copy=0x1,
            DW_LNS_advance_pc=0x2,
            DW_LNS_advance_line=0x3,
            DW_LNS_set_file=0x4,
            DW_LNS_set_column=0x5,
            DW_LNS_negate_stmt=0x6,
            DW_LNS_set_basic_block=0x7,
            DW_LNS_const_add_pc=0x8,
            DW_LNS_fixed_advance_pc=0x9,
            DW_LNS_set_prologue_end=0xa,
            DW_LNS_set_epilogue_begin=0xb,
            DW_LNS_set_isa=0xc
        };

        static constexpr std::string_view opcodes_str[13]{
            "Extended",
            "DW_LNS_copy",
            "DW_LNS_advance_pc",
            "DW_LNS_advance_line",
            "DW_LNS_set_file",
            "DW_LNS_set_column",
            "DW_LNS_negate_stmt",
            "DW_LNS_set_basic_block",
            "DW_LNS_const_add_pc",
            "DW_LNS_fixed_advance_pc",
            "DW_LNS_set_prologue_end",
            "DW_LNS_set_epilogue_begin",
            "DW_LNS_set_isa",
        };

        // 00
        // uleb128 len (subcode + payload)
        // u8 subcode
        // payload
        enum ExtendedOpcodes {
            DW_LNE_end_seq=0x1,
            DW_LNE_set_address=0x2,
            DW_LNE_define_file=0x3,
            DW_LNE_set_discriminator=0x4,
        };

        static constexpr std::string_view extended_opcodes_str[5]{
            "",
            "DW_LNE_end_seq",
            "DW_LNE_set_address",
            "DW_LNE_define_file",
            "DW_LNE_set_discriminator"
        };

        inline void parse_header_v4(const std::vector<char>& f, size_t offset){
            auto ptr = f.data() + offset;
            DebugLineHeaderV4 dlh;
            memcpy(&dlh, ptr, sizeof(dlh));
            dlh.print();
            
            auto vm_bytecode_ptr = dlh.prologue_length + ptr + 10;
            finish_address = (unsigned char*)(ptr + dlh.unit_length + 4);
            is_stmt = dlh.is_stmt;
            line_base = dlh.line_base;
            line_range = dlh.line_range;
            opcode_base = dlh.opcode_base;
            min_inst_length = dlh.min_inst_length;
            ptr += sizeof(DebugLineHeaderV4);
        #if DEBUG    
            print_hex(ptr, 64);
        #endif    
            for (int i = 0, e = opcode_base-1; i != e; ++i)
                lists.standart_opcode_len.push_back(*ptr++);
            std::string s;
            for(;;){
                s.clear();
                while(*ptr) s += *ptr++;
                if (s == "") {++ptr; s.clear(); break;};
                lists.include_directories.push_back(s);
            }
            s.clear();
            while(*ptr){
                while(*ptr) s += *ptr++;
                std::cout << "Dir: " << (void*) ptr << '\n';
                size_t dir_index = decode_leb128<size_t>(ptr);
                std::cout << "modification_time: " << (void*) ptr << '\n';
                size_t modification_time = decode_leb128<size_t>(ptr);
                std::cout << "file_len: " << (void*) ptr << '\n';
                size_t file_len = decode_leb128<size_t>(ptr);
                lists.file_entries.push_back({DebugLineParseVM::FileTableEntry{s, dir_index, modification_time, file_len}});
            }
            ++ptr;
        #if DEBUG
            lists.print_standart_opcode_len();
            lists.print_include_directories();
            lists.print_file_entries();
        #endif

            assert(*ptr == 0);
            ++ptr;
            print_hex(ptr, 64);
            assert(ptr == vm_bytecode_ptr);
            it = (unsigned char*)ptr;
            exec();
            std::cout << "Exec finish FileSize:" << lists.file_entries.size() << '\n';
            std::cout << "CommitedStates:\n";
            for (const auto& el: commited_states) el.print();
            std::cout << "FINISH!\n";
        }

        inline void parse_header_v5(const std::vector<char>& f, size_t debug_line_offset, size_t debug_line_str_offset){
            auto ptr = f.data() + debug_line_offset;
            auto debug_str_ptr = f.data() + debug_line_str_offset;  
        #if DEBUG
            print_hex(ptr, 128);
        #endif    
            DebugLineHeaderV5 dlh;
            memcpy(&dlh, ptr, sizeof(dlh));
            dlh.print();
            auto vm_bytecode_ptr = dlh.prologue_length + ptr + 10;
            finish_address = (unsigned char*)(ptr + dlh.unit_length + 4);
            is_stmt = dlh.is_stmt;
            line_base = dlh.line_base;
            line_range = dlh.line_range;
            opcode_base = dlh.opcode_base;
            min_inst_length = dlh.min_inst_length;
            ptr += sizeof(DebugLineHeaderV5);
        #if DEBUG    
            print_hex(ptr, 64);
        #endif
            for (int i = 0, e = opcode_base-1; i != e; ++i)
                lists.standart_opcode_len.push_back(*ptr++);
        
            unsigned char directory_entry_format_count = *ptr++;
            std::cout << "directory_entry_format_count:" << (int)directory_entry_format_count << '\n';
            
            std::vector<FieldV5> dir_entry_struct;
            for (size_t i = 0; i < directory_entry_format_count; ++i){
                auto content_type = decode_leb128<unsigned>(ptr);
                auto form = decode_leb128<unsigned>(ptr);
                dir_entry_struct.push_back(FieldV5{(ContentTypeV5)content_type, (FormV5)form});
            }
            std::cout << "Entry:\n";
            for (auto& el: dir_entry_struct){
                std::cout << to_sv(el.content_type) << " " << to_sv(el.form) << '\n';
            }

            unsigned char directory_entry_sz = *ptr++;
            std::cout << "directory_entry_size:" << (int)directory_entry_sz << '\n';
            for (size_t i = 0; i < directory_entry_sz; ++i){
                if (dir_entry_struct[0].form == DW_FORM_line_strp){
                    unsigned str_offset = read<unsigned>(ptr); // todo: check sz
                    lists.include_directories.emplace_back(std::string(f.data() + debug_line_str_offset + str_offset));
                }
            }

            for (const auto& el: lists.include_directories){
                std::cout << el << '\n';
            }
            
            auto file_name_entry_format_count = decode_leb128<unsigned>(ptr);
            std::cout << "file_name_entry_format_count:" << file_name_entry_format_count << '\n';
            std::vector<FieldV5> file_entry_struct;
            for (size_t i = 0; i < file_name_entry_format_count; ++i){
                auto content_type = decode_leb128<unsigned>(ptr);
                auto form = decode_leb128<unsigned>(ptr);
                file_entry_struct.push_back(FieldV5{(ContentTypeV5)content_type, (FormV5)form});
            }
            std::cout << "File format:\n";
            for (auto& el: file_entry_struct){
                std::cout << to_sv(el.content_type) << " " << to_sv(el.form) << '\n';
            }

            unsigned char file_entry_sz = *ptr++;
            std::cout << "file_entry_sz:" << (int)file_entry_sz << '\n';

            FileTableEntry fte;
            for (size_t i = 0; i < file_entry_sz; ++i){
                for (auto& el: file_entry_struct){
                    if (el.content_type == DW_LNCT_path && el.form == DW_FORM_line_strp){
                        unsigned str_offset = read<unsigned>(ptr); // todo: check sz
                        fte.name = (f.data() + debug_line_str_offset + str_offset);
                    } else if (el.content_type == DW_LNCT_dir_index && el.form == DW_FORM_udata){
                        fte.dir_index = decode_leb128<unsigned>(ptr);
                    }
                }
                lists.file_entries.push_back(fte);
            }

            std::cout << "File entries:\n";
            for (auto& el: lists.file_entries)
                std::cout << "Name:" << el.name << " " << " Dir index:" << el.dir_index << '\n';

        

            ++ptr;
            print_hex(ptr, 64);
            // assert(ptr == vm_bytecode_ptr);
            it = (unsigned char*)ptr;
            exec();
            std::cout << "Exec finish FileSize:" << lists.file_entries.size() << '\n';
            std::cout << "CommitedStates:\n";
            for (const auto& el: commited_states) el.print();
            std::cout << "FINISH!\n";
        }

        inline void parse_header(const std::vector<char>& f, size_t debug_line_offset, size_t debug_line_str_offset = 0){
            auto ptr = f.data() + debug_line_offset;
            DebugLineHeader dlh;
            memcpy(&dlh, ptr, sizeof(dlh));
            if (dlh.version == 4){
                parse_header_v4(f, debug_line_offset);
            } else if (dlh.version == 5){
                parse_header_v5(f, debug_line_offset, debug_line_str_offset);
            }
        }

        void* get_address_by_file_and_line(const std::string& filename, size_t line){
            int current_file_index = -1;
            for (size_t i = 0, e = lists.file_entries.size(); i != e; ++i){
                if (lists.file_entries[i].name == filename){
                    current_file_index = i+1;
                }
            }
            if (current_file_index == -1) return nullptr;
            for (const auto& el: commited_states){
                if (el.file == current_file_index && el.line == line){
                    return (void*)el.address;
                }
            }
            return nullptr;
        }

        inline void exec(){
            state.set_flag(DebugLineParseVM::State::IsStmt, is_stmt);
            while(it <= finish_address){
                std::cout << "Exec FileSize:" << lists.file_entries.size() << '\n';
#if DEBUG
            std::cout << "Opcode:(" << std::setw(2) << std::setfill('0') << unsigned(*it) << ") - ";
            std::cout <<  (*it <= 12 ? opcodes_str[*it] : "Special") << std::endl;
#endif
                switch(*it){
                    case 0: {++it;extended_opcode_exec(); break;};
                    case DW_LNS_copy:               {++it;  commited_states.push_back(state); break;}
                    case DW_LNS_advance_pc:         {++it;  state.address += min_inst_length * decode_leb128<unsigned long>(it); break;}
                    case DW_LNS_advance_line:       {++it;  state.line += decode_leb128<long>(it); break;}
                    case DW_LNS_set_file:           {++it;  state.file = decode_leb128<unsigned long>(it);   break;}
                    case DW_LNS_set_column:         {++it;  state.column = decode_leb128<unsigned long>(it); break;}
                    case DW_LNS_negate_stmt:        {++it;  state.toggle_flag(State::IsStmt); break;}
                    case DW_LNS_set_basic_block:    {++it;  state.set_flag(State::BasicBlock, true); ++it; break;}
                    case DW_LNS_const_add_pc:       {++it;  state.address += (opcode_base - 1) * min_inst_length; break;}
                    case DW_LNS_fixed_advance_pc:   {++it;  state.address += read<unsigned>(it); break;}
                    case DW_LNS_set_prologue_end:   {++it;  state.set_flag(State::PrologueEnd, true); break;}
                    case DW_LNS_set_epilogue_begin: {++it;  state.set_flag(State::EpilogueBegin, true);break;}
                    case DW_LNS_set_isa:            {++it;  isa = decode_leb128<unsigned long>(it); break;}
                    default: {special_opcode_exec();++it; break;}
                }
            }
            std::cout << "Exec finish FileSize:" << lists.file_entries.size() << '\n';
        }

        inline void extended_opcode_exec(){
            size_t len = decode_leb128<unsigned>(it);
            auto ptr = it + len;
#if DEBUG            
            std::cout << "Opcode:(" << std::setw(2) << std::setfill('0') << unsigned(*it) << ") - ";
            if ((*it <= 4)) std::cout << extended_opcodes_str[*it];
            
            std::cout <<  '\n';
#endif
            switch(*it++){
                case DW_LNE_end_seq: {
                    state.set_flag(State::EndSequence, true); 
                    commited_states.push_back(state); state = State{}; 
                    state.set_flag(DebugLineParseVM::State::IsStmt, is_stmt);
                    break;}
                case DW_LNE_set_discriminator: {/*todo*/ break;}
                case DW_LNE_define_file: {
                    auto name = read_str(it);  
                    size_t dir_index = decode_leb128<size_t>(it);
                    size_t modification_time = decode_leb128<size_t>(it);
                    size_t file_len = decode_leb128<size_t>(it);
                    lists.file_entries.push_back(FileTableEntry{name, dir_index, modification_time, file_len});
                    break;}
                case DW_LNE_set_address: {
                    state.address = read<unsigned long>(it);
#if DEBUG                    
                    std::cout << "Adress:" << state.address << '\n';
#endif                
                    break;}
            }
            assert(it == ptr);
        }

        inline void special_opcode_exec(){
            auto val = *it - opcode_base;
            int line_inc = line_base + (val % line_range);
            auto addr_inc = (val/line_range) * min_inst_length;
            state.address += addr_inc;
            state.line += line_inc;
            commited_states.push_back(state);
        }
};

#if defined(__APPLE__)
inline void print_dwarf_cu_header_macho(const std::vector<char>& f, Section64 sc){
    auto ptr = f.data() + sc.offset;
    CompilationUnitHeader cuh;
    memcpy(&cuh, ptr, sizeof(cuh));
    std::cout << std::hex;
    std::cout << " Unit length:" << cuh.unit_length << '\n';
    std::cout << " Version:" << cuh.version << '\n';
    std::cout << " DebugAbbrevOffset:" << cuh.debug_abbrev_offset << '\n';
    std::cout << " Address sz:" << (int)cuh.address_sz << "\n\n";
}
#endif
#endif