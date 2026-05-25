#include <iostream>
#include <dlfcn.h>
#include <vector>
#include <variant>
#include <cstring>

void* load_lib(const std::string& s){
    auto handle = dlopen(s.c_str(), RTLD_LAZY);
    if (!handle) {
        std::cout << "Error:" << dlerror() << '\n';
        return nullptr;
    }
    return handle;
}

void* load_function(void* handle, const std::string& s){
    auto res = dlsym(handle, s.c_str());
    if (!res){
        std::cout << "Error:" << dlerror() << '\n';
        return nullptr;
    }
    return res;
}

struct CLI {
    enum Mode: unsigned char { DynLoad, DynCall, Help, Exit, ModeCount};
    static constexpr std::string_view mode_str[4] = {
        "DynLoad", "DynCall", "Help", "Exit"
    };
    Mode mode = Help;

    enum ArgType: unsigned char {Int,String,Float};
    static constexpr std::string_view arg_type_str[3] = {
        "Int", "String","Float"
    };

    enum IntRegs{RDI, RSI, RCX, RDX, R8, R9};
        static constexpr std::string_view regs_int_str[6] = {
        "RDI", "RSI", "RCX", "RDX", "R8", "R9"
    };

    enum FloatRegs{XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7};
        static constexpr std::string_view regs_float_str[8] = {
        "XMM0","XMM1","XMM2","XMM3","XMM4","XMM5","XMM6","XMM7"
    };

    enum RegType{IntReg,FloatReg};

    static auto get_int_arg_obj(size_t imm, size_t i){
        std::array<unsigned char, 10> res;
        switch(i){
            case RDI: {res[0] = 0x48; res[1] = 0xbf; break;}
            case RSI: {res[0] = 0x48; res[1] = 0xbe; break;}
            case RCX: {res[0] = 0x48; res[1] = 0xb9; break;}
            case RDX: {res[0] = 0x48; res[1] = 0xba; break;}
            case R8:  {res[0] = 0x49; res[1] = 0xb8; break;}
            case R9:  {res[0] = 0x49; res[1] = 0xb9; break;}
        }
        memcpy(res.data()+2, &imm, sizeof(size_t));
        return res;
    }

    static auto get_float_arg_obj(double imm, size_t i){
        static constexpr unsigned char xmm_i_instr[8]{
            0xc7,  // xmm0
            0xcf,  // xmm1
            0xd7,  // xmm2
            0xdf,  // xmm3
            0xe7,  // xmm4
            0xef,  // xmm5
            0xf7,  // xmm6
            0xff,  // xmm7
        };
        std::array<unsigned char, 15> res;
        auto i_imm = get_int_arg_obj(std::bit_cast<size_t>(imm), 0);
        std::copy(i_imm.begin(), i_imm.end(), res.begin());
        // 66 48 0f 7e * -> movq rdi, xmm(0-7)
        res[10] = 0x66;
        res[11] = 0x48;
        res[12] = 0x0f;
        res[13] = 0x7e;
        res[14] = xmm_i_instr[i];
        return res;
    }

    static auto get_function_call_instr(void* ptr){
        std::array<unsigned char, 12> res;
        res[0] = 0x48;
        res[1] = 0xb8; // mov rax, ptr
        memcpy(res.data()+2, &ptr, sizeof(void*));
        res[10] = 0xff;
        res[11] = 0xd0; // call rax
        return res;
    }


    // todo: finish + test
    static auto generate_call_instructions(const std::string& s){
        std::vector<unsigned char> res;
        res.push_back(0x55); // push rbp

        res.push_back(0x48); 
        res.push_back(0x89);
        res.push_back(0xe5); // push rpb, rsp
                    
        auto args = parse_args(s);
        for (const auto [t, sv, rt, rv]: args) {
            auto s = std::string(sv);
            if (t == Int){
                auto ins = get_int_arg_obj(std::atoi(s.c_str()), rv);
                for (auto el: ins) res.push_back(el);
            } else if (t == Float) {
                auto ins = get_float_arg_obj(std::stof(s.c_str()), rv);
                for (auto el: ins) res.push_back(el);
            } else if (t == String) {
                auto ins = get_int_arg_obj((std::uintptr_t)s.c_str(), rv);
                for (auto el: ins) res.push_back(el);
            }
        }
        res.push_back(0xc3); // ret
        return res;
    }

    static ArgType get_type(std::string_view sv){
        if (std::any_of(sv.begin(), sv.end(),[](auto el){return el == '.';}))
            return Float;
        else if (std::any_of(sv.begin(), sv.end(),[](auto el){return el == '\"';}))
            return String;
        else 
            return Int;
    }

    static std::vector<std::tuple<ArgType,std::string_view, RegType, unsigned char>> parse_args(std::string_view s) {
        std::vector<std::tuple<ArgType,std::string_view, RegType, unsigned char>> res;
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
            if (t == Float){
                res.push_back(std::tuple{t, res_sv, FloatReg, regs_float_count++});
            } else {
                res.push_back(std::tuple{t, res_sv, IntReg, regs_int_count++});
            }
        }
        auto res_sv = std::string_view{s.begin() + i, s.end()};
        auto t = get_type(res_sv);
        if (t == Float){
            res.push_back(std::tuple{t, res_sv, FloatReg, regs_float_count++});
        } else {
            res.push_back(std::tuple{t, res_sv, IntReg, regs_int_count++});
        }
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

    void update_mode(unsigned char s){
        if (s >= ModeCount){mode = Help; return;}
        mode = static_cast<Mode>(s);
    }

    void print_mode_info(){
        std::cout << "\nCurrentMode:";
        std::cout << mode_str[mode] << '\n';
        switch(mode){
            case DynLoad: {std::cout << "path to lib:\n";break;}
            case DynCall: {std::cout << "function sym\n";break;}
            case Help:    {std::cout << "Pick mode:\n0: DynLoad\n1: DynCall\n2: Help\n3: Exit\n";break;}
        }
    }
    
    std::vector<std::string> split(const std::string& s){
        std::vector<std::string> vec;
        size_t pos = 0;
        for (int i = 0, e = s.size(); i != e; ++i){
            if (s[i] == ' '){
                vec.push_back(std::string(s.begin() + pos, s.begin() + pos + i));
                pos = i;
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

        if (strs.size() > 1){
            print_args(parse_args(std::string{std::find(s.begin(), s.end(), ' ') + 1, s.end()}));
        }
    
        switch(mode){
            case Help: {update_mode(std::atoi(strs[0].c_str())); break;}
            case DynLoad: {lib_handle = nullptr; lib_handle = load_lib(strs[0]); if(lib_handle){current_lib_path = strs[0]; update_mode(DynCall);} break;}
            case DynCall: {
                function_handler = nullptr; 
                function_handler = (void(*)(...))load_function(lib_handle, strs[0]);
                if (function_handler){
                    // todo: generalized
                    if (strs.size() > 1){
                        auto args = parse_args(std::string_view{std::find(s.begin(), s.end(), ' ') + 1, s.end()});
                        if (args.size() == 1) {
                            auto [t, sv, rt, rv] = args[0];
                            std::string str(sv);
                            switch(t){
                                case Int:    function_handler(std::atoi(str.c_str())); break;
                                case Float:  function_handler(std::stof(str.c_str())); break;
                                case String: function_handler(str.c_str()); break;
                            }
                        }
                    } else {
                        function_handler();
                    }
                }; break;}
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

// TODO: try mmap exec
int main(){
    CLI cli;
    cli.run();
}
