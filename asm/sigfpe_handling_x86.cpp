#define _XOPEN_SOURCE 700
#include <iostream>
#include <cassert>
#include <ucontext.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
                        

void division_by_zero_handler(int sig, siginfo_t* info, void* context) {
    auto ctx = static_cast<ucontext_t*>(context);
    ctx->uc_mcontext->__ss.__r8 = 2; // R8 = 2
}

int main() {
    struct sigaction sa{};
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = division_by_zero_handler;
    sigaction(SIGFPE, &sa, nullptr);

    unsigned long long rax{};

    // Division by zero 
    asm volatile (
        "mov $10, %%rax\n\t"  // RAX = 10
        "mov $0, %%r8\n\t"    // R8 = 0
        "idiv %%r8\n\t"       // After handler R8: 2 ==> RAX: 5 (10/2)
        "mov %%rax, %0"
        : "=r"(rax)
        :: "rax", "r8", "memory"
    );
    assert(rax == 5);
}