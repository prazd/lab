#include <execinfo.h>
#include <iostream>

void print_stack_c_style() {
    void* callstack[128];
    int frames = backtrace(callstack, 128);
    char** strs = backtrace_symbols(callstack, frames);
    for (int i = 0; i < frames; ++i) {
        std::cout << strs[i] << '\n';
    }
    free(strs);
}

void f2(){print_stack_c_style();}
void f1(){f2();}

int main() {
   f1();
}