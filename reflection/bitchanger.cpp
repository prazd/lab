#include <iostream>
#include <meta>
#include <string_view>
#include <typeinfo>
#include <cassert>

template <typename T>
struct BitChanger {
    static inline constexpr auto fields = 
        std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
    T* s_ptr;

    struct Iter {
        T* base;
        unsigned char* addr;
        size_t n = 0;

        // TODO: finish all random access methods
        bool operator==(const Iter& it)const{return addr == it.addr && n == it.n;}
        bool operator==(bool val){return operator*() == val;}
        bool operator*() {return ((*addr) & (1 << n));}
        Iter& operator++(){
            bool next_field = false;
            template for (constexpr auto value : fields){
                auto field_ptr = (unsigned char*)&(base->[:value:]);
                if (next_field){
                    addr = field_ptr;
                    next_field = false;
                    n = 0;
                    break;
                }
                if (addr >= field_ptr && addr <= field_ptr + (sizeof(base->[:value:]) - 1)){
                    if (addr == field_ptr + (sizeof(base->[:value:]) - 1) && n == 7) {
                        next_field = true; 
                        continue;
                    } else if (n == 7){
                        ++addr; n = 0; break;
                    }
                    ++n;
                    break;
                }
            }
            if (next_field) {++addr; n = 0;}
            return *this;
        }
        Iter& operator=(bool val){
            *addr &= (~(1 << n));
            *addr |= (val << n);
            return *this;
        }
    };
    
    Iter operator[](size_t i){
        auto byte_count = i/8;
        auto bit_index = i%8;
        unsigned char* addr = 0;
        template for (constexpr auto value : fields){
            if (auto sz = sizeof(s_ptr->[:value:]); byte_count >= sz){
                byte_count -= sz;
                continue;
            }
            addr = ((unsigned char*)(&(s_ptr->[:value:]))) + byte_count;
            break;
        }
        return Iter{s_ptr, addr, bit_index};
    }

    auto begin(){return Iter{s_ptr, (unsigned char*)s_ptr,0};}
    auto end(){return Iter{s_ptr,(unsigned char*)(s_ptr+1),0};}
};

struct S {
    unsigned char x = 0b1010'1011;
    unsigned short y = 0b0000'1111'1111'0000;
    unsigned long z = 0xe;
};

int main(){
    S s;
    BitChanger bc(&s);
    std::cout << "Begin:\n";
    for (auto el: bc) std::cout << el;
    std::cout << "\n\nbc[2] = 1\nbc[8] = 1\nbc[24] = 1\n";
    
    bc[2] = 1;
    assert(s.x & 0xf);
    bc[8] = 1;
    assert(s.y & 1);
    bc[24] = 1;
    assert(s.z & 1);

    std::cout << "\nRes:\n";

    for (auto el: bc) std::cout << el;;
}