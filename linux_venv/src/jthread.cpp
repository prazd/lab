#include <iostream>
#include <thread>
#include <chrono>

void test () {
    std::jthread jt([](){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "Finish" << std::endl;
    });
    throw 1;
}

int main() {
    try {test();}catch(int i){std::cout << "Error:" << i;}
}