#include <ctime>
#include <iostream>
#include <stdlib.h>
#include <sys/uio.h>
#include <stdio.h>
#include <string>
#include <array>
#include <cstring>
#include <vector>
#include <charconv>
#include <algorithm>

enum Command {
    Unknown,
    Info,
    ClearVec,
    Search,
    SetPid,
    SetRange,
    Print,
    Write,
    Close,
    CmdCount,
};
constexpr std::string_view cmd_sv_arr[CmdCount] = {
    "unknown", "info", "clear_vec", "search", "set_pid", "set_range", "print", "write", "close"
};

Command parse_cmd(const std::string_view sv) {
    for (size_t i = 0; i != CmdCount; ++i) {
        if (sv == cmd_sv_arr[i])
            return (Command)i;
    }
    return Unknown;
}

template <size_t sz>
auto split_to_sv(const std::string& s){
    std::array<std::string_view, sz> res;
    auto prev = s.begin();
    auto ptr = s.begin();
    size_t i = 0;
    for (auto e = s.end(); ptr != e;){
        if (*std::next(ptr) == ' ' || std::next(ptr) == e){
            res[i++] = std::string_view(prev, ptr+1);
            ++ptr;
            while(*ptr == ' ') ++ptr;
            prev = ptr;
        } else {
            ++ptr;
        }
    }
    return res;
}

struct CLI {
    std::string s;
    std::vector<void*> addresses;
    unsigned long begin{};
    unsigned long end{};
    int p_pid{};

    void search(unsigned long value){
        unsigned long v = 0;
        
        iovec local;
        local.iov_base = &v;
        local.iov_len = sizeof(v); 

        iovec remote;
        remote.iov_len = sizeof(v);
        auto b = begin;
        auto e = end;

        if (addresses.empty()) {
            for (;b!=e;b+=8) {
                v = 0;
                remote.iov_base = (void*)b;
                ssize_t nread = process_vm_readv(p_pid, &local, 1, &remote, 1, 0);
                if (nread == -1){
                    perror("process_vm_readv error");
                    break;
                }
                if (v == value) addresses.push_back((void*)b);
            }
        } else {
            for (auto& addr: addresses) {
                v = 0;
                remote.iov_base = (void*)addr;
                ssize_t nread = process_vm_readv(p_pid, &local, 1, &remote, 1, 0);
                if (nread == -1){
                    perror("process_vm_readv error");
                }
                if (v != value) addr = 0;
            }
            addresses.erase(std::remove(addresses.begin(), addresses.end(), (void*)0), addresses.end());
        }
    }

    void run(){
        for(;;){
            std::cout << ">> ";
            std::getline(std::cin, s);
            auto svs = split_to_sv<4>(s);
            switch(parse_cmd(svs[0])){
                case Info: {
                    std::cout << "Begin:" << std::hex << begin << '\n'
                              << "End:" << end << '\n'
                              << "PID:" << std::dec <<  p_pid << '\n'
                              << "Addresses sz:" << addresses.size() << '\n';
                    break;
                }
                case ClearVec: {
                    addresses.clear(); 
                    std::cout << "addresses sz:" << addresses.size() << '\n';
                    break;
                }
                case Print: {
                    size_t arg_sz = 0;
                    std::from_chars(svs[1].data(), svs[1].data() + svs[1].size(), arg_sz);
                    auto e = std::min(addresses.size(), arg_sz);
                    for (size_t i = 0; i != e; ++i)
                        std::cout << '[' << i << "]:" << std::hex << addresses[i] << std::dec << '\n';
                    break;
                }
                case SetPid: {
                    std::from_chars(svs[1].data(), svs[1].data() + svs[1].size(), p_pid);
                    break;
                }
                case SetRange: { 
                    std::cout << "Begin:" << svs[1] << '\n';
                    std::cout << "End:" << svs[2] << '\n';
                    std::from_chars(svs[1].data(), svs[1].data() + svs[1].size(), begin, 16);
                    std::from_chars(svs[2].data(), svs[2].data() + svs[2].size(), end, 16);
                    break;
                }
                case Search: {
                    unsigned long value = 0;
                    std::from_chars(svs[1].data(), svs[1].data() + svs[1].size(), value, 16);
                    search(value);
                    break;
                }
                // TODO: finish
                case Write: std::cout << "Write\n"; break;
                case Close: std::cout << "Close\n"; break;
                default:{
                    std::cout << "Unknown\n";
                }
            }
        }   
    }

};


// setcap cap_sys_ptrace=eip ./a.out
int main(int argc, char* argv[]){
    CLI{}.run();
}
