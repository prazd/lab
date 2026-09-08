#include <iostream>
#include <chrono>
#include <thread>


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
};

int main(){
    Frequency f;
    std::cout << "RDTSC:" << f.rdtsc() << " GHz\n";
    std::cout << "CPUID (base)" << f.cpuid() << " GHz\n";
}