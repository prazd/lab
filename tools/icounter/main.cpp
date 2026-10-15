#include <iostream>
#include <cassert>
#include <unistd.h>
#include <utility>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <chrono>
#include <thread>
#include <iomanip>
#include <array>
#include <cstring>
#include <climits>
#include <vector>


user_regs_struct read_regs(int pid){
    user_regs_struct regs{};
    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) std::abort();
    return regs;
}

enum WaitStatus {Exited,Error,Stopped};
WaitStatus wait(int pid){
    int status{};
    int ret = waitpid(pid, &status,0);
    if (ret == -1) return Error;
    else if (WIFEXITED(status)) return Exited;
    else if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP){
        return Stopped;
    }
    return Error;
}

unsigned char c_to_nibble(char c){
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return (c - 'a') + 10;
    return (c-'A') + 10;
}

class Instruction {
    unsigned char buf[15]{};
    size_t sz{};
public:
    Instruction(std::string_view sv):sz((sv.size() - (sv[0] == '0' && sv[1] == 'x' ? 2 : 0))/2){
        std::string_view i_str = (sv[0] == '0' && sv[1] == 'x') ? sv.substr(2) : sv;
        auto ptr = i_str.begin();
        for (size_t i = 0; i < sz; ++i)
            buf[i] = (c_to_nibble(*ptr++) << 4) | c_to_nibble(*ptr++);
    }
    void reverse(){
        unsigned char* rp = buf;
        unsigned char* lp = rp + (sz-1);
        while(rp != lp){
            std::swap(*rp, *lp);
            --lp;
            if (rp == lp) break;
            ++rp;
        }
    }
    bool equal(const std::array<unsigned char, 15>& arr) const {
        return std::equal(arr.begin(), arr.begin() + sz, buf, buf+sz);
    }
    size_t size() const {return sz;}

    std::string_view to_sv() const {
        return {(char*)buf, sz};
    }

    auto begin(){return buf;}
    auto end(){return buf+sz;}

};


template <size_t sz>
auto read_instruction(int pid){
    std::array<unsigned char, 15> res{};
    unsigned long val{};
    auto rip = read_regs(pid).rip;
    auto data = ptrace(PTRACE_PEEKDATA, pid, rip, 0);
    memcpy(res.data(), &data, 8);
    if constexpr (sz > 8){
        data = ptrace(PTRACE_PEEKDATA, pid, rip+8, 0);
        memcpy(res.data()+8, &data, sz-8);
    }
    return res;
}

template <typename It>
void print_hex(It it, It ite){
    std::cout << std::hex;
    auto print_nibble = [](unsigned char nibble){
        if (nibble >= 0 && nibble <= 9) std::cout << (char)('0' + nibble);
        else std::cout << (char)('a' + (nibble - 10));
    };
    for (;it != ite;++it){
        auto v = *it;
        print_nibble((v >> 4) & 0xf);
        print_nibble(v & 0xf);
    }
    std::cout << std::dec;
}

int main(int argc, char* argv[]){
    assert(argc >= 3);
    auto pid = fork();
    char* args[] = {NULL};
    char* envs[] = {NULL};
    if (pid == 0){
        ptrace(PT_TRACE_ME, 0, 0, 0);
        execve(argv[1], args, envs); // TODO: handle args + env
    } else {
        waitpid(pid, 0, 0);
        std::vector<std::pair<Instruction,size_t>> instructions{};
        
        for (size_t i = 2; i != argc; ++i){
            Instruction instruction{argv[i]};
            if (i + 1 != argc && std::string_view(argv[i + 1]) == "le") {instruction.reverse();++i;}
            instructions.push_back({instruction, 0});
        }

        std::cout << "Instructions:\n";
        size_t instruction_n = 0;
        for (auto& [v, _]: instructions){
            std::cout << '[' << instruction_n++ << "]: "; 
            print_hex(v.begin(), v.end());
            std::cout << '(' << v.size() << ")\n";
        }

        size_t max_i_sz{};
        for (size_t i = 0, e = instructions.size(); i != e; ++i){
            auto& [v,_] = instructions[i];
            max_i_sz = v.size() > max_i_sz ? v.size() : max_i_sz;
        }
        auto read_f = max_i_sz > 8 ? read_instruction<15> : read_instruction<8>;
        if (read_f == read_instruction<15>)
            std::cout << "Read function version - 15b\n";
        else 
            std::cout << "Read function version - 8b\n";

        std::cout << "In progress...\n";
        for(;;){
            auto current_instruction = read_f(pid);
            for (auto& [v,c]: instructions)
                c += (v.equal(current_instruction));
            ptrace(PTRACE_SINGLESTEP, pid, 0, 0);
            if (auto res = wait(pid); res != Stopped) break;
        }

        std::cout << "\n\nResult:\n";
        for (auto& [v,c]: instructions){
            print_hex(v.begin(), v.end());
            std::cout << ':' << c << '\n';
        }
    }
}