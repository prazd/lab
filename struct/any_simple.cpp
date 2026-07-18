#include <iostream>
#include <stdlib.h>

template <typename T>
concept Trivial = std::is_trivially_copyable_v<T> && sizeof(T) <= sizeof(void*) && alignof(T) <= (sizeof(void*));

struct any {
    using destroy_f = void(*)(void*);
    void* addr = nullptr;
    union {
        alignas(void*) unsigned char buffer[8]{};
        void(*destroy)(void*);
    };

    template <typename T>
    void construct_trivial(T&& val){
        new (buffer) T(std::forward<T>(val));
        addr = (void*)(((std::uintptr_t)(addr)) | 1);
    }

    template <typename T>
    void construct(T&& val){
        addr = aligned_alloc(alignof(T), sizeof(T));
        new (addr) T(std::forward<T>(val));
        destroy = [](void* val){((T*)(val))->~T();free(val);};
    }

    any(){destroy = [](void*){};}

    template <Trivial T> 
    any(T&& val){construct_trivial(std::forward<T>(val));}

    template <typename T> 
    any(T&& val){construct(std::forward<T>(val));}

    template <typename T>
    any& operator=(T&& val) {
        if (!(((std::uintptr_t)(addr)) & 1))
            destroy(addr);
        construct(std::forward<T>(val));
        return *this;
    }

    template <Trivial T>
    any& operator=(T&& val) {
        if (!(((std::uintptr_t)(addr)) & 1))
            destroy(addr);
        construct_trivial(std::forward<T>(val));
        return *this;
    }

    template <typename T>
    T& cast(){return *reinterpret_cast<T*>(addr);}

    template <Trivial T>
    T& cast(){return *reinterpret_cast<T*>(buffer);}
    
    ~any(){if (!((std::uintptr_t)(addr) & 1)) destroy(addr);}
};

void* operator new(size_t n){
    printf("Alloc:%zu\n", n);
    return malloc(n);
}

int main(){
    any x;
    x = 123;
    std::cout << x.cast<int>() << '\n';
    x = std::string("abababababbababababa");
    std::cout << x.cast<std::string>() << '\n';
}