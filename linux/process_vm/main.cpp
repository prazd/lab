#include <chrono>
#include <thread>
#include <iomanip>
#include <unistd.h>
#include <iostream>

int main(){
    unsigned long v = 0x1122334455667788;
    while(v != 0x42){
        std::cout << "PID:" << getpid() << '\n' << 
                   "Addr:" << &v << '\n' << 
                    std::hex  << "V:" << v << std::dec <<  '\n'; 
       std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
       std::cout << "\033[H\033[J";
    };
    std::cout << std::hex << "DONE!\nV:" << v << '\n'; 
}
