#include <iostream>

#pragma section my_data_section read write
int my_data __attribute__((section(",my_data_section"))) = 42;


int main(){
    std::cout << "My data section:" << my_data;
}
