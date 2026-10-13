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

// TODO: check
unsigned long be_to_le(unsigned long value, size_t sz){
    unsigned char* rp = (unsigned char*)(&value);
    unsigned char* lp = rp + (sz-1);
    while(rp != lp){
        std::swap(*rp, *lp);
        --lp;
        if (rp == lp) break;
        ++rp;
    }
    return value;
}

unsigned long mask_by_i_sz(size_t sz){
    return (0xff'ff'ff'ff'ff'ff'ff'ff >> ((8 - sz) * CHAR_BIT));
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
        std::string_view i_sv = argv[2];
        // TODO: > 8 bytes instruction handling
        unsigned long i_ul = std::stoul(i_sv.data(), nullptr, 16);
        size_t i_sz = (i_sv.size() - (i_sv[0] == '0' && i_sv[1] == 'x' ? 2 : 0))/2;
        if (argv[3]){
            std::string_view i_mode = argv[3];
            if (i_mode == "be") i_ul = be_to_le(i_ul, i_sz);
        }
        std::cout << "Instruction:" << i_sv << '\n';
        std::cout << "Size:" << i_sz << '\n';
        unsigned long rip{};

        unsigned long data{};
        unsigned long instruction{};
        unsigned long mask = mask_by_i_sz(i_sz);
        std::cout << "Mask:" << std::hex << mask << std::dec << '\n';
        std::cout << "Start...\n";
        for(;;){
            rip = read_regs(pid).rip;
            data = ptrace(PTRACE_PEEKDATA, pid, rip, 0);
            instruction = i_ul;
            data &= mask;
            instruction &= mask;
            counter += (data == instruction);
            ptrace(PTRACE_SINGLESTEP, pid, 0, 0);
            if (auto res = wait(pid); res != Stopped) break;
        }
        print("Count:", counter,'\n');
    }
}