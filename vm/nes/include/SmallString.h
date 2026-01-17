#ifndef NES_SMALLSTRING_H
#define NES_SMALLSTRING_H
#include <memory_resource>
template <size_t n>
class SmallString {
    char buffer[n * 2]{};
    std::pmr::monotonic_buffer_resource mbr{buffer, sizeof(buffer), std::pmr::null_memory_resource()};
    std::pmr::string s;
public:
    inline SmallString(): s(&mbr){s.reserve(n);}
    inline SmallString(const char* str): s{str, &mbr} {s.reserve(n);}
    SmallString& operator+=(char c){
        s+= c;
        return *this;
    }
    SmallString& operator+=(std::string_view str){
        s += str;
        return *this;
    }
    SmallString& operator+=(const char* str){
        s += str;
        return *this;
    }
    inline SmallString& operator=(const char* str) {
        s = str;
        return *this;
    }
    inline char* data() {
        return s.data();
    }
    inline char* buf() {
        return buffer;
    }
};
#endif //NES_SMALLSTRING_H
