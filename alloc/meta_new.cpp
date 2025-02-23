#include <iostream>


template <typename T>
void print(T& t) {
   auto ptr = (unsigned char*)&t;
   for (auto it = ptr + sizeof(t) - 1, ite = ptr-1; it != ite; --it)
	  std::cout << (void*)it << " " << (unsigned)(*it) << '\n';
}

int main() {
    // linux
    auto x = new char[400];
    *x = 10;
    auto nptr = (long*)x;
    print(*(nptr-1));
    std::cout << "Size:" << nptr[-1] << "\n";
    delete[] x;
}