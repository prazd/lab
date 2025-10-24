#include <iostream>
#include <cstring>
#include <utility>
#include <cassert>

class CowString {
        struct CowStringBuf {
            size_t cap{};
            size_t sz{};
            char* ptr{};
            int refcount{};
            constexpr CowStringBuf(char* str, size_t sz, size_t cap): ptr(str), sz(sz), cap(cap), refcount(1){}
            constexpr CowStringBuf(const char* str): cap(strlen(str) + 1), sz(cap-1), ptr(new char[cap]{}), refcount(1) {
                std::copy(str, str + strlen(str), ptr);
            }
            constexpr CowStringBuf(const CowStringBuf& rhs): cap(rhs.cap), sz(rhs.sz), ptr(new char[cap]{}), refcount(1) {
                std::copy(rhs.ptr, rhs.ptr + rhs.sz, ptr);
            }

            constexpr CowStringBuf(CowStringBuf&& rhs) noexcept: cap(rhs.cap),
                    sz(rhs.sz), 
                    ptr(std::exchange(rhs.ptr, nullptr)),
                    refcount(rhs.refcount){}

            constexpr CowStringBuf& operator=(CowStringBuf rhs) {
                if (&rhs == this)
                    return *this;
                std::swap(rhs.cap, cap);
                std::swap(rhs.sz, sz);
                std::swap(rhs.ptr, ptr);
                std::swap(rhs.refcount, refcount);
                return *this;
            }

            constexpr size_t capacity() const {
                return cap;
            }
            constexpr size_t size() const {
                return sz;
            }
            constexpr ~CowStringBuf(){
                delete[] ptr;
            }
    };

    constexpr auto get_new_buffer(const char* it, const char* ite) {
        auto cur_sz = ptr->size();
        auto other_sz = std::distance(it, ite);
        size_t cap = cur_sz + other_sz + 1;
        char* str = new char[cap]{};
        std::copy(ptr->ptr, ptr->ptr + cur_sz, str);
        std::copy(it, ite, str + cur_sz);
        return new CowStringBuf(str, cap-1, cap);
    }

    constexpr void free_buffer() {
        if (ptr->refcount == 1)
            delete ptr;
        else 
            --(ptr->refcount);
    }
public:    
    CowStringBuf* ptr;
    constexpr CowString(CowStringBuf* ptr): ptr(ptr){}
    constexpr CowString(const char* str): ptr(new CowStringBuf(str)){}
    constexpr CowString(const CowString& rhs): ptr(rhs.ptr){
        ++(ptr->refcount);
    }
    constexpr CowString(CowString&& rhs) noexcept: ptr(std::exchange(rhs.ptr, nullptr)){}
    
    constexpr CowString& operator=(const CowString& other) {
        if (&other == this) return *this;
        free_buffer();
        ptr = other.ptr;
        ++(ptr->refcount);
        return *this;
    }

    constexpr CowString& operator=(CowString&& other) {
        if (&other == this) return *this;
        free_buffer();
        ptr = std::exchange(other.ptr, nullptr);
        return *this;
    }

    constexpr const char* c_str() const {
        return ptr->ptr;
    }
    
    constexpr CowString operator+(const CowString& rhs) {
        return CowString{get_new_buffer(rhs.data(), rhs.data() + rhs.size())};
    } 
    
    constexpr CowString& operator+=(const CowString& rhs) {
        auto tmp = get_new_buffer(rhs.data(), rhs.data() + rhs.size());
        free_buffer();
        ptr = tmp;
        return *this;
    }
    
    constexpr CowString& operator+=(const char* rhs) {
        auto tmp = get_new_buffer(rhs, rhs + strlen(rhs));
        free_buffer();
        ptr = tmp;
        return *this;
    }
    constexpr const char* data() const {
        return ptr->ptr;
    }
    constexpr size_t capacity() const {
        return ptr->capacity();
    }
    constexpr size_t size() const {
        return ptr->size();
    }
    constexpr ~CowString(){
        free_buffer();
    }
};

std::ostream& operator<<(std::ostream& o, const CowString& cs) {
    o << cs.data();
    return o;
}

int main(){
    static_assert([](){
        CowString x("abc"); 
        return x.size();
    }() == 3);
    
    static_assert([](){
        CowString x("abc"); 
        x += "123"; 
        return x.size();
    }() == 6);

    static_assert([](){
        CowString x("abc"); 
        CowString y("xyz"); 
        x+=y; 
        return x.size();
    }() == 6);

    static_assert([](){
        CowString x("abc"); 
        CowString y("xyz"); 
        x=y; 
        assert(std::equal(x.c_str(), x.c_str() + x.size(), y.c_str()));
        return x.size();
    }() == 3);

    static_assert([](){
        CowString x("abc"); 
        CowString y("xyz"); 
        x=y; 
        assert(std::equal(x.c_str(), x.c_str() + x.size(), y.c_str()));
        return x.size();
    }() == 3);

    static_assert([](){
        CowString x("abc");
        CowString y = x;
        CowString z = x;
        return x.ptr->refcount;
    }() == 3);
    static_assert([](){
        CowString x("abc");
        CowString y = x;
        x += "123";
        assert(std::equal(x.c_str(), x.c_str() + x.size(), "abc123"));
        assert(x.ptr->refcount == 1);
        assert(std::equal(y.c_str(), y.c_str() + y.size(), "abc"));
        assert(y.ptr->refcount == 1);
        return x.ptr->refcount;
    }() == 1);
}