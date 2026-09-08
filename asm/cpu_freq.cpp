#include <iostream>
#include <chrono>
#include <thread>
#if defined(__linux__)
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#endif
struct Frequency { 
   unsigned long regs[4]{};
  
   float rdtsc(){ 
      asm volatile("lfence");
      asm volatile("rdtsc" : "=a"(regs[0]), "=d"(regs[1]));
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      asm volatile("rdtscp" : "=a"(regs[2]), "=d"(regs[3]));
      asm volatile("lfence");
      return ((regs[2] | (regs[3] << 32)) - (regs[0] | (regs[1] << 32))) / 1000'000'000.f;
   }

   float cpuid() {
      //                        Base MHz       Max MHZ         Bus MHz
      asm volatile ("cpuid" : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]) : "a"(0x16)); 
      return regs[0] / 1000.f;
   }
#if defined(__linux__)
   // TODO: run on linux
   unsigned long read_msr_n(char n, unsigned char offset) {
       static char msr_path[] = "/dev/cpu/n/msr";
       msr_path[9] = n;
       int fd = open(msr_path, O_RDONLY);
       unsigned long res{};
       if (fd < 0) std::abort();
       pread(fd, &res, 8, offset);
       close(fd);
       return res;
   }
   double rdmsr() {
       double base = 2.3; //  
       auto mperf0 = read_msr_n('1', 0xe7);
       auto apref0 = read_msr_n('1', 0xe8);
       std::this_thread::sleep_for(std::chrono::milliseconds(100));
       auto mperf1 = read_msr_n('1', 0xe7);
       auto apref1 = read_msr_n('1', 0xe8);
       unsigned long dm = mperf1-mperf0;
       unsigned long da = apref1-apref0;
       return ((double)da/(double)dm) * base; //
   }
#endif

};

int main(){
    Frequency f;
    std::cout << "RDTSC:" << f.rdtsc() << " GHz\n";
    std::cout << "CPUID(base):" << f.cpuid() << " GHz\n";
#if defined(__linux__)    
    std::cout << "RDMSR:" << f.rdmsr() << " GHz\n";
#endif
}