#include <iostream>
extern "C" int add(int,int);
int main(){
    std::cout << add(20,22);
}
