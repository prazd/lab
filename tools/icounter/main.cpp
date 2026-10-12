#include <iostream>
#include <cassert>
#include <unistd.h>
#include <utility>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <chrono>
#include <thread>

user_regs_struct read_regs(int pid){
    user_regs_struct regs{};
    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) std::abort();
    return regs;
}

template <typename ... Args>
void print(Args&& ... args){
    std::cout << "\033c";
    (std::cout << ... << args);
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
        size_t i_sz = i_sv.size() - (i_sv[0] == '0' && i_sv[1] == 'x' ? 2 : 0);
        unsigned long rip{};
        bool equal = true;
        for(;;){
            ptrace(PTRACE_SINGLESTEP, pid, 0, 0);
            waitpid(pid, 0, 0);
            rip = read_regs(pid).rip;
            unsigned long data = ptrace(PTRACE_PEEKDATA, pid, rip, 0);
            for (size_t i = 0; i < i_sz; ++i){
                if ((data & 0xff) != (i_ul & 0xff)) equal = false;
                data >>= 8;
                i_ul >>= 8;
            }
            counter += std::exchange(equal, true);
            print("Count:", counter,'\n');
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // debug
        }
    }
}