#include <iostream>
#include <dlfcn.h>
#include <vector>
#include <variant>
#include <cstring>
#include <array>
#include <algorithm>
#include <sys/mman.h>
#include <unistd.h>

#define DEBUG 0

void* load_lib(const std::string& s){
    auto handle = dlopen(s.c_str(), RTLD_LAZY);
    if (!handle) {
        std::cout << "Error:" << dlerror() << '\n';
        return nullptr;
    }
    return handle;
}

void* load_sym(void* handle, const std::string& s){
    auto res = dlsym(handle, s.c_str());
    if (!res){
        std::cout << "Error:" << dlerror() << '\n';
        return nullptr;
    }
    return res;
}

void mmap_execute(const std::vector<unsigned char>& instrs){ 
    void* addr = mmap(NULL, instrs.size(), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr != MAP_FAILED){
        std::copy(instrs.begin(), instrs.end(), (unsigned char*)addr);
        auto f = (void(*)())addr;
        f();
        munmap(addr, instrs.size());
    }
}

struct CLI {
    enum Mode: unsigned char { DynLoad, DynCall, DynVarRead, Help, Exit, ModeCount};
    static constexpr std::string_view mode_str[5] = {
        "DynLoad", "DynCall", "DynVarRead", "Help", "Exit"
    };
    Mode mode = Help;

    enum ArgType: unsigned char {Int,String,Float,Double};
    static constexpr std::string_view arg_type_str[4] = {
        "Int", "String","Float","Double"
    };

    enum IntRegs{RDI, RSI, RDX, RCX, R8, R9};
        static constexpr std::string_view regs_int_str[6] = {
        "RDI", "RSI", "RDX", "RCX", "R8", "R9"
    };

    enum FloatRegs{XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7};
        static constexpr std::string_view regs_float_str[8] = {
        "XMM0","XMM1","XMM2","XMM3","XMM4","XMM5","XMM6","XMM7"
    };

    static constexpr unsigned char xmm_rax_instr[8]{
            0xc0,  // xmm0_rax
            0xc8,  // xmm1_rax
            0xd0,  // xmm2_rax
            0xd8,  // xmm3_rax
            0xe0,  // xmm4_rax
            0xe8,  // xmm5_rax
            0xf0,  // xmm6_rax
            0xf8,  // xmm7_rax
    };

    enum RegType{IntReg,FloatReg};

    static auto get_int_arg_obj(size_t imm, size_t i){
        std::array<unsigned char, 10> res;
        switch(i){
            case RDI: {res[0] = 0x48; res[1] = 0xbf; break;}
            case RSI: {res[0] = 0x48; res[1] = 0xbe; break;}
            case RDX: {res[0] = 0x48; res[1] = 0xba; break;}
            case RCX: {res[0] = 0x48; res[1] = 0xb9; break;}
            case R8:  {res[0] = 0x49; res[1] = 0xb8; break;}
            case R9:  {res[0] = 0x49; res[1] = 0xb9; break;}
        }
        memcpy(res.data()+2, &imm, sizeof(size_t));
        return res;
    }

    static std::array<unsigned char, 18> get_arg_rsp_obj(size_t imm, int rsp_index){
        std::array<unsigned char, 18> res;
        auto mov_rax_ins = get_int_arg_rax_obj(imm); // rax = imm
        std::copy(std::begin(mov_rax_ins), std::end(mov_rax_ins), res.begin());
        rsp_index *= 8;
        res[10] = 0x48;
        res[11] = 0x89;
        res[12] = 0x84; 
        res[13] = 0x24;// dword ptr[rsp + (rsp_index)] = rax
        auto arr = std::bit_cast<std::array<unsigned char, sizeof(rsp_index)>>(rsp_index);
        std::copy(std::begin(arr), std::end(arr), res.begin() + 14);
        return res;
    }

    static auto get_arg_rsp_obj(double imm, int rsp_index){
        size_t imm_i = 0;
        memcpy(&imm_i, &imm, sizeof(imm_i));
        return get_arg_rsp_obj(imm_i, rsp_index);
    }

    static std::array<unsigned char, 10> get_int_arg_rax_obj(size_t imm){
        std::array<unsigned char, 10> res;
        res[0] = 0x48;
        res[1] = 0xb8;
        memcpy(res.data()+2, &imm, sizeof(size_t));
        return res;
    }

    static std::array<unsigned char, 15> get_double_arg_obj(double imm, size_t i){
        std::array<unsigned char, 15> res;
        auto i_imm = get_int_arg_rax_obj(std::bit_cast<size_t>(imm));
        std::copy(i_imm.begin(), i_imm.end(), res.begin());
        // 66 48 0f 6e [*] -> movq rax, xmm(0-7)
        res[10] = 0x66;
        res[11] = 0x48;
        res[12] = 0x0f;
        res[13] = 0x6e;
        res[14] = xmm_rax_instr[i];
        return res;
    }

    static auto get_float_arg_obj(float imm, size_t i){
        std::array<unsigned char, 15> res;
        size_t imm_i = 0;
        memcpy(&imm_i, &imm, sizeof(float));
        auto i_imm = get_int_arg_rax_obj(imm_i);
        std::copy(i_imm.begin(), i_imm.end(), res.begin());
        // 66 48 0f 6e * -> movq rax, xmm(0-7)
        res[10] = 0x66;
        res[11] = 0x48;
        res[12] = 0x0f;
        res[13] = 0x6e;
        res[14] = xmm_rax_instr[i];
        return res;
    }

    static auto get_function_call_instr(void* ptr){
        std::array<unsigned char, 12> res;
        res[0] = 0x48;
        res[1] = 0xb8; // mov rax, ptr
        auto addr = std::bit_cast<std::array<unsigned char, sizeof(void*)>>(ptr);
        std::copy(addr.begin(), addr.end(), res.data()+2);
        res[10] = 0xff;
        res[11] = 0xd0; // call rax
        return res;
    }

    static auto add_rsp_ins(int offset){
        std::array<unsigned char, 7> res;
        res[0] = 0x48;
        res[1] = 0x81;
        res[2] = 0xc4;
        memcpy(res.data() + 3, &offset, sizeof(offset));
        return res;
    }

    static auto sub_rsp_ins(int offset){
        std::array<unsigned char, 7> res;
        res[0] = 0x48;
        res[1] = 0x81;
        res[2] = 0xec;
        memcpy(res.data() + 3, &offset, sizeof(offset));
        return res;
    }

    static auto count_ins_type(const auto& args){
        size_t f_type = 0;
        size_t i_type = 0;
        for (auto it = args.begin() + 1, ite = args.end(); it != ite; ++it) {
            auto [t, sv] = *it;
            i_type += (t == Int || t == String) ? 1 : 0;
            f_type += (t == Float || t == Double) ? 1 : 0;
        }
        return std::pair{f_type, i_type};
    }

    static auto generate_call_instructions(const std::string& s, void* function_addr){
        static std::vector<std::string> str_args;
        str_args.clear();

        auto parse_int = [](std::string_view sv){
            size_t res = 0;
            if (std::any_of(sv.begin(), sv.end(), [](auto el){return el == 'x';})){
                size_t i = 0;
                for (size_t e = sv.size(); i != e; ++i){
                    if (sv[i] == 'x') {++i; break;}
                }
                std::string s(sv.substr(i));
                res = std::stoul(s, 0 , 16);                
            } else {
                std::string s(sv);
                res = std::atoi(s.c_str());
            }
            return res;
        };

        std::vector<unsigned char> res;
        res.push_back(0x55); // push rbp

        res.push_back(0x48); 
        res.push_back(0x89);
        res.push_back(0xe5); // mov rpb, rsp
                    
        unsigned char f_count = 0;
        unsigned char i_count = 0;
        size_t rsp_index = 0;
        auto args = parse_args(s);
        auto [d_ins_count, i_ins_count] = count_ins_type(args);

        bool stack_args = (i_ins_count > 6 || d_ins_count > 8);

        int rsp_offset = 0;
        if (stack_args){
            if (i_ins_count > 6){
                rsp_offset += i_ins_count - 6;
            }
            if (d_ins_count > 8){
                rsp_offset += d_ins_count - 8;
            }
            rsp_offset *= 8;
            auto ins = sub_rsp_ins(rsp_offset);
            std::copy(ins.begin(), ins.end(), std::back_inserter(res));
        }

        for (auto it = args.begin() + 1, ite = args.end(); it != ite; ++it){
            const auto [t, sv] = *it;
            auto str = std::string(sv);
            if (t == Int){
                if (i_count == 6) {
                    auto ins = get_arg_rsp_obj(parse_int(sv), rsp_index++);
                    std::copy(ins.begin(), ins.end(), std::back_inserter(res));
                } else {
                    auto ins = get_int_arg_obj(parse_int(sv), i_count++);
                    std::copy(ins.begin(), ins.end(), std::back_inserter(res));
                }
            } else if (t == Double) {
                if (f_count == 8){
                    auto ins = get_arg_rsp_obj(std::stod(str.c_str()), rsp_index++);
                    std::copy(ins.begin(), ins.end(), std::back_inserter(res));
                } else {
                    auto ins = get_double_arg_obj(std::stod(str.c_str()), f_count++);
                    std::copy(ins.begin(), ins.end(), std::back_inserter(res));
                }
            } else if (t == Float) {
                if (f_count == 8){
                    auto ins = get_arg_rsp_obj(std::stod(str.c_str()), rsp_index++);
                    std::copy(ins.begin(), ins.end(), std::back_inserter(res));
                } else {
                    auto ins = get_float_arg_obj(std::stof(str.c_str()), f_count++);
                    std::copy(ins.begin(), ins.end(), std::back_inserter(res));
                }
            } else if (t == String) {
                str_args.push_back(str);
                if (i_count == 6) {
                    auto ins = get_arg_rsp_obj((std::uintptr_t)str_args.back().data(), rsp_index++);
                    std::copy(ins.begin(), ins.end(), std::back_inserter(res));
                } else {
                    auto ins = get_int_arg_obj((std::uintptr_t)str_args.back().data(), i_count++);
                    std::copy(ins.begin(), ins.end(), std::back_inserter(res));
                }
            }
        }
        auto f_call_ins = get_function_call_instr(function_addr);
        for (auto el: f_call_ins) res.push_back(el);
        if (stack_args){
            auto ins = add_rsp_ins(rsp_offset);
            std::copy(ins.begin(), ins.end(), std::back_inserter(res));
        }
        res.push_back(0x5d); // pop rbp
        res.push_back(0xc3); // ret
#if DEBUG
        std::cout << "Size:" << std::dec << res.size() << '\n';
        for (auto el: res){
            std::cout << std::hex << (int)el << '\n';
        }
        std::cout << std::endl;
#endif        
        return res;
    }

    static ArgType get_type(std::string_view sv){
        if (std::any_of(sv.begin(), sv.end(),[](auto el){return el == '\"';}))
            return String;
        else if (std::any_of(sv.begin(), sv.end(),[](auto el){return el == 'x';}))
            return Int;
        else if (std::any_of(sv.begin(), sv.end(),[](auto el){return el == 'f';}))
            return Float;
        else if (std::any_of(sv.begin(), sv.end(),[](auto el){return el == 'd';}))
            return Double;
        else 
            return Int;
    }

    static std::vector<std::pair<ArgType,std::string_view>> parse_args(std::string_view s) {
        std::vector<std::pair<ArgType,std::string_view>> res;
        unsigned char regs_int_count = 0;
        unsigned char regs_float_count = 0;
        size_t i = 0;
        size_t e = s.size();
        while(i<e){
            auto it = std::find(s.begin() + i, s.end(), ' ');
            if (it == std::end(s)) break;
            auto res_sv = std::string_view{s.begin() + i, it};
            i+=res_sv.size()+1;
            auto t = get_type(res_sv);
            res.push_back(std::pair{t, res_sv});
        }
        auto res_sv = std::string_view{s.begin() + i, s.end()};
        auto t = get_type(res_sv);
        res.push_back(std::pair{t, res_sv});
        return res;
    }

    void print_args(const auto& args){
        std::cout << "Args:\n";
        for (auto& [t, str, rt, rv]: args){
            std::cout << arg_type_str[t] << " " << str << " ";
            if (rt == FloatReg)
                std::cout << regs_float_str[rv];
            else
                std::cout << regs_int_str[rv];
            std::cout << '\n';
        }
        std::cout << '\n';
    }

    std::string current_lib_path{};
    void* lib_handle = nullptr;

    void(*function_handler)(...) = nullptr; // todo: custom function
    void* global_var_addr = nullptr;

    void update_mode(unsigned char s){
        if (s >= ModeCount){mode = Help; return;}
        mode = static_cast<Mode>(s);
    }

    void print_mode_info(){
        std::cout << "\nCurrentMode:";
        std::cout << mode_str[mode] << '\n';
        switch(mode){
            case DynLoad:    {std::cout << "path to lib:\n";break;}
            case DynCall:    {std::cout << "function sym\n";break;}
            case DynVarRead: {std::cout << "variable sym and type: (WHITE int)\n"; break;}
            case Help:       {std::cout << "Pick mode:\n0: DynLoad\n1: DynCall\n2: DynVarRead\n3: Help\n4: Exit\n";break;}
        }
    }
    
    std::vector<std::string> split(const std::string& s){
        std::vector<std::string> vec;
        size_t pos = 0;
        for (int i = 0, e = s.size(); i != e; ++i){
            if (s[i] == ' '){
                vec.push_back(std::string(s.begin() + pos, s.begin() + pos + i));
                pos = i + 1;
            }
        }
        vec.push_back(std::string(s.begin() + pos, s.end()));
        return vec;
    }
    void handle(const std::string& s){
        if (s == "help"){
           update_mode(Help);
           return;
        }
        auto strs = split(s);
        switch(mode){
            case Help: {update_mode(std::atoi(strs[0].c_str())); break;}
            case DynLoad: {lib_handle = nullptr; lib_handle = load_lib(strs[0]); if(lib_handle){current_lib_path = strs[0]; update_mode(DynCall);} break;}
            case DynCall: {
                function_handler = nullptr; 
                function_handler = (void(*)(...))load_sym(lib_handle, strs[0]);
                if (function_handler)
                    mmap_execute(generate_call_instructions(s, (void*)function_handler));
                break;                   
                }
            case DynVarRead: {
                    global_var_addr = nullptr;
                    global_var_addr = load_sym(lib_handle, strs[0]);
                    if (!global_var_addr) break;

                    if (strs[1] == std::string_view{"int"}){
                        unsigned i = 0;
                        memcpy(&i, global_var_addr, sizeof(unsigned));
                        std::cout << "Var:" << i << '\n';
                    } else if (strs[1] == std::string_view{"long"}) {
                        size_t i = 0;
                        memcpy(&i, global_var_addr, sizeof(size_t));
                        std::cout << "Var:" << i << '\n';
                    } else if (strs[1] == std::string_view{"float"}){
                        float f = 0;
                        memcpy(&f, global_var_addr, sizeof(float));
                        std::cout << "Var:" << f << '\n';
                    } else if (strs[1] == std::string_view{"double"}){
                        double d = 0;
                        memcpy(&d, global_var_addr, sizeof(double));
                        std::cout << "Var:" << d << '\n';
                    }
                }                 
            }
    }

    void run(){
        std::string cmd;
         for (;;){
           print_mode_info();  
           std::cout << ">> ";
           std::getline(std::cin, cmd);
           handle(cmd);
           cmd.clear();
           if (mode == Exit) return;
       }
    }
};

int main(){
    CLI cli;
    cli.run();
}