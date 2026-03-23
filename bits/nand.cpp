consteval bool nand(bool a, bool b){
    return !(a & b);
}

consteval bool cand(bool a, bool b){
    return nand(nand(a,b), nand(a,b));
}

consteval bool cor(bool a, bool b){
    return nand(nand(a,a), nand(b,b));
}

consteval bool cxor1(bool a, bool b){
    return nand((cor(cand(nand(a,a), nand(b,b)), cand(a, b))), (cor(cand(nand(a,a), nand(b,b)), cand(a, b))));
}
 
consteval bool cxor2(bool a, bool b){
    return nand(nand(a, nand(a,b)), nand(b, nand(a,b)));
}

int main(){
    static_assert(nand(1,1) == 0);
    static_assert(nand(1,0) == 1);
    static_assert(nand(0,1) == 1);
    static_assert(nand(0,0) == 1);

    static_assert(cand(1,1) == 1);
    static_assert(cand(1,0) == 0);
    static_assert(cand(0,1) == 0);
    static_assert(cand(0,0) == 0);

    static_assert(cxor1(1,1) == 0);
    static_assert(cxor1(1,0) == 1);
    static_assert(cxor1(0,1) == 1);
    static_assert(cxor1(0,0) == 0);

    static_assert(cxor2(1,1) == 0);
    static_assert(cxor2(1,0) == 1);
    static_assert(cxor2(0,1) == 1);
    static_assert(cxor2(0,0) == 0);

    static_assert(cor(1,1) == 1);
    static_assert(cor(1,0) == 1);
    static_assert(cor(0,1) == 1);
    static_assert(cor(0,0) == 0);
}