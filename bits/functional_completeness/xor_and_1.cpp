consteval bool cxor(bool a, bool b){
    return a ^ b;
}

consteval bool cand(bool a, bool b){
    return a & b;
}

consteval bool cnot(bool a) {
    return cxor(a,1);
}

consteval bool nand(bool a, bool b) {
    return cnot(cand(a,b));
}

consteval bool cor(bool a, bool b) {
    return cxor(cxor(a,b), cand(a,b));
}

consteval bool cor2(bool a, bool b) {
    return nand(cnot(cxor(a,b)), nand(a,b));
}

int main(){
    static_assert(cxor(1,1) == 0);
    static_assert(cxor(1,0) == 1);
    static_assert(cxor(0,1) == 1);
    static_assert(cxor(0,0) == 0);

    static_assert(cand(1,1) == 1);
    static_assert(cand(1,0) == 0);
    static_assert(cand(0,1) == 0);
    static_assert(cand(0,0) == 0);

    static_assert(cnot(0) == 1);
    static_assert(cnot(1) == 0);

    static_assert(nand(1,1) == 0);
    static_assert(nand(1,0) == 1);
    static_assert(nand(0,1) == 1);
    static_assert(nand(0,0) == 1);

    static_assert(cor(1,1) == 1);
    static_assert(cor(1,0) == 1);
    static_assert(cor(0,1) == 1);
    static_assert(cor(0,0) == 0);

    static_assert(cor2(1,1) == 1);
    static_assert(cor2(1,0) == 1);
    static_assert(cor2(0,1) == 1);
    static_assert(cor2(0,0) == 0);
}