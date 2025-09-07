struct A {
    int x;
    long y;
};

#pragma pack(push, 1)
struct B {
    int x;
    long y;
};
#pragma pack(pop)

// Or several
#pragma pack(push, 1)
struct C1 {
    char x;
    short y;
};
struct C2 {
    char x;
    short y;
};
#pragma pack(pop)

struct __attribute__((packed)) D {
    char x;
    long y;
};

struct __attribute__((aligned(32))) E {
    long x;
    long y;
};

int main() {
    static_assert(sizeof(A)   == 16);
    static_assert(sizeof(B)   == 12);
    static_assert(sizeof(C1)  == 3);
    static_assert(sizeof(C2)  == 3);
    static_assert(sizeof(D)   == 9);
    static_assert(sizeof(E)   == 32);
}