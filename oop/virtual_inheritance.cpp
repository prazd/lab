#include <iostream>

struct Grandma {
    int x;
    Grandma(int x): x(x/4){std::cout << "GrandmaCtor";}
};

struct Dad: virtual Grandma {
    int d;
    Dad(int d): Grandma(d), d((d/2)+1) {std::cout << "DadCtor";}
};

struct Mom: virtual Grandma {
    int m;
    Mom(int m): Grandma(m), m((m/2)+2){std::cout << "MomCtor";}
};

struct Son: Dad, Mom {
    int s;
    Son(int s): Grandma(s), Dad(s), Mom(s), s(s) {std::cout << "SonCtor" << std::endl;}
};

int main() {
    Son s(200);
    auto ptr = reinterpret_cast<unsigned char*>(&s);
    for(int i = sizeof(s)-1;i>=0;--i) {
        std::cout << (void*)(ptr + i) << ": " << (int)(ptr[i])  << std::endl;
    }
}