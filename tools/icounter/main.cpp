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


user_regs_struct read_regs(int pid){
    user_regs_struct regs{};
    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) std::abort();
    return regs;
}

template <typename ... Args>
void print(Args&& ... args){
    (std::cout << ... << args);
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
        return std::equal(arr.begin(), arr.end(), buf);
    }
    size_t size() const {return sz;}
    std::string_view to_sv() const {
        return {(char*)buf, sz};
    }
};

auto read_instruction(int pid, size_t sz){
    std::array<unsigned char, 15> res{};
    unsigned long val{};
    auto rip = read_regs(pid).rip;
    auto data = ptrace(PTRACE_PEEKDATA, pid, rip, 0);
    memcpy(res.data(), &data, std::min(sz,8zu));
    if (sz > 8){
        data = ptrace(PTRACE_PEEKDATA, pid, rip+8, 0);
        memcpy(res.data()+8, &data, sz-8);
    }
    return res;
}

int main(int argc, char* argv[]){
    assert(argc >= 2);
    auto pid = fork();
    char* args[] = {NULL};
    char* envs[] = {NULL};
    if (pid == 0){
        ptrace(PT_TRACE_ME, 0, 0, 0);
        execve(argv[1], args, envs); // TODO: handle args + env
    } else {
        waitpid(pid, 0, 0);
        size_t counter = 0;
        Instruction instruction{argv[2]};
        if (argv[3]){
            std::string_view i_mode = argv[3];
            if (i_mode == "le") instruction.reverse();
        }
        std::cout << "Instruction:" << argv[2] << '\n';
        std::cout << "Size:" << instruction.size() << '\n';
        std::cout << "In progress...\n";
        for(;;){
            counter += (instruction.equal(read_instruction(pid, instruction.size())));
            ptrace(PTRACE_SINGLESTEP, pid, 0, 0);
            if (auto res = wait(pid); res != Stopped) break;
        }
        print("Count:", counter,'\n');
    }
}