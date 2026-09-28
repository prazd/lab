#include <iostream>
#include <chrono>

constexpr size_t RUNS_COUNT = 1000'000;
constexpr size_t SZ = RUNS_COUNT << 7;
unsigned char ARR[SZ];

#define NOP asm("nop");
#define NOP2    NOP     NOP
#define NOP4    NOP2    NOP2
#define NOP8    NOP4    NOP4
#define NOP16   NOP8    NOP8
#define NOP32   NOP16   NOP16
#define NOP64   NOP32   NOP32
#define NOP128  NOP64   NOP64

#define NOP160  NOP128   NOP32
#define NOP176  NOP160   NOP16
#define NOP180  NOP176   NOP4
#define NOP188  NOP180   NOP8
#define NOP196  NOP188   NOP8
#define NOP212  NOP196   NOP16
#define NOP220  NOP212   NOP8

#define NOP256  NOP128  NOP128
#define NOP512  NOP256  NOP256
#define NOP1024 NOP512  NOP512
#define NOP2048 NOP1024 NOP1024

#define ROB_SZ(N)\
    void rob_sz_##N(size_t i){\
        asm("movb (%0), %%al"::"r"(ARR+i)    :"rax","memory");\
        NOP##N\
        asm("movb (%0), %%al"::"r"(ARR + (i+64)) :"rax","memory");\
    }\

ROB_SZ(2)
ROB_SZ(4)
ROB_SZ(8)
ROB_SZ(16)
ROB_SZ(32)
ROB_SZ(64)
ROB_SZ(128)
ROB_SZ(160)
ROB_SZ(176)
ROB_SZ(180)
ROB_SZ(188)
ROB_SZ(196)
ROB_SZ(212)
ROB_SZ(220)
ROB_SZ(256)
ROB_SZ(512)
ROB_SZ(1024)
ROB_SZ(2048)

unsigned long bench(auto rob_f){
    unsigned long begin_l{};
    unsigned long begin_h{};
    unsigned long end_l{};
    unsigned long end_h{};
    
    double res = 0.0;
    for (size_t i = 0; i < RUNS_COUNT; ++i){
        asm volatile("lfence");
        asm volatile("rdtsc": "=a"(begin_l), "=d"(begin_h));

        asm("movb (%0), %%al"::"r"(ARR + ((i * 128) +64)) :"rax","memory"); // hot load

        rob_f(i * 128);
        asm volatile("rdtscp" : "=a"(end_l), "=d"(end_h));
        asm volatile("lfence");
        res += ((end_l | (end_h << 32)) - (begin_l | (begin_h << 32)));
    }
    return (res/RUNS_COUNT);
}

void flush_cache(){
    for (size_t i = 0; i < SZ; i += 64) {
        asm("clflush %0"::"m"(*(ARR + i)) : "memory");
    }
}

#define RUN_N_PRINT_ROB_SZ(N)  std::cout << "ROB_SZ_"#N": " << bench(rob_sz_##N) << "\n"; flush_cache();

int main(){
    std::generate(ARR, ARR + SZ, []{return rand() & 0xff;});
    RUN_N_PRINT_ROB_SZ(2)
    RUN_N_PRINT_ROB_SZ(4)
    RUN_N_PRINT_ROB_SZ(8)
    RUN_N_PRINT_ROB_SZ(16)
    RUN_N_PRINT_ROB_SZ(32)
    RUN_N_PRINT_ROB_SZ(64)
    RUN_N_PRINT_ROB_SZ(128)
    RUN_N_PRINT_ROB_SZ(160)
    RUN_N_PRINT_ROB_SZ(176)
    RUN_N_PRINT_ROB_SZ(180)
    RUN_N_PRINT_ROB_SZ(188)
    RUN_N_PRINT_ROB_SZ(196)
    RUN_N_PRINT_ROB_SZ(212)
    RUN_N_PRINT_ROB_SZ(220)
    RUN_N_PRINT_ROB_SZ(256)
    RUN_N_PRINT_ROB_SZ(512)
    RUN_N_PRINT_ROB_SZ(1024)
    RUN_N_PRINT_ROB_SZ(2048)
}