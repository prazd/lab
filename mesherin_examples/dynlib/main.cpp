#include <iostream>

int func(int);


int main() { 
   extern int z;
   std::cout << func(z) << "\n";
}
