#include <iostream>
#include <string>
#include <memory_resource>
#include <cstdio>

// TODO: debug gcc version
// g++ -std=c++2b small_string.cpp
template <typename T, size_t N>
struct ArenaAllocator {
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template<class U>
    struct rebind
    {
        typedef ArenaAllocator<U, N> other;
    };

    T memory_resource[N * sizeof(T) * 2]{};
    T* current = memory_resource;

    T* allocate(size_t n){
        std::cout << "ADDR:" << (void*)memory_resource << '\n';
        std::cout << "SZ:" << sizeof(memory_resource) << '\n';
        std::cout << "N:" << n << '\n';
        auto res = current;
        current += n;
        return res;
    }

    void deallocate(T* p, size_t n){}
};


template <size_t n>
class SmallString {
    std::basic_string<char, std::char_traits<char>, ArenaAllocator<char, n>> bs;
public:
    SmallString(const char* str): bs(str){}
    SmallString& operator=(const char* str) {
        bs = str;
        return *this;
    }
    char* data() {
        return bs.data();
    }
};

template <size_t n>
class SmallStringPmr {
    char buffer[n * 2]{};
    std::pmr::monotonic_buffer_resource mbr{buffer, sizeof(buffer), std::pmr::null_memory_resource()};
    std::pmr::string s;
public:
    SmallStringPmr(): s{&mbr}{}
    SmallStringPmr& operator+=(const char* str){
        s += str;
        return *this;
    }
    SmallStringPmr(const char* str): s{str, &mbr}{}
    SmallStringPmr& operator=(const char* str) {
        s = str;
        return *this;
    }
    char* data() {
        return s.data();
    }
    char* buf() {
        return buffer;
    }
};

int main() {
    SmallStringPmr<40> s1 = "aaaaaaaaaaaaaaa";
    s1 += "bbbbbbbbbbbbb";
    SmallString<31> s2 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbb";
    std::cout << "S1:" << (void*)s1.data() << " " << s1.data() << '\n';
    std::cout << "S1 buffer addr:" << (void*)s1.buf() << '\n';
    std::cout << "S2 addr:" << &s2 << '\n';
    std::cout << "S2:" << (void*)s2.data() << " " << s2.data() << '\n';
}