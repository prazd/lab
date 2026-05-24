#include <iostream>
#include <dlfcn.h>
#include <vector>

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
        
        switch(mode){
            case Help: {update_mode(std::atoi(strs[0].c_str())); break;}
            case DynLoad: {lib_handle = nullptr; lib_handle = load_lib(strs[0]); if(lib_handle){current_lib_path = strs[0]; update_mode(DynCall);} break;}
            case DynCall: {
                function_handler = nullptr; 
                function_handler = (void(*)(...))load_function(lib_handle, strs[0]);
                if (function_handler){
                    // todo: generalized
                    if (strs.size() > 1){
                        function_handler(std::atoi(strs[1].c_str()));
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

int main(){
    CLI cli;
    cli.run();
}
