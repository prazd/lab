#ifndef SMD_SIMPLE_HPP
#define SMD_SIMPLE_HPP
#include <array>

template <size_t cap>
struct SmallString {
    char buf[cap]{};
    size_t sz = 0;
    SmallString() = default;
    explicit SmallString(std::string_view sv){for (auto c: sv) push(c);}
    void reset(){sz = 0;}
    void push(char c){buf[sz++] = c;}
    auto begin(){return buf;}
    auto end(){return buf + sz;}
    std::string_view to_sv() {
        return std::string_view(buf, sz);
    }
    SmallString& operator+(const SmallString& rhs){
        for (size_t i = 0; i < rhs.sz; ++i) push(rhs.buf[i]);
        return *this;
    }
};

template <size_t sz>
auto ztoarr(size_t val){
    struct Result {
        std::array<char, sz> arr;
        size_t c_count{};
    } r;
    size_t n = 0;
    auto ptr = r.arr.begin();
    while(val){
        n = val % 10;
        *ptr++ = 0x30 + n;
        val/=10;
        ++r.c_count;
    }
    return r;
}

template <size_t sz, size_t num_c_max = 10, char sym = '&'>
SmallString<sz> str_num_template(std::string_view t, size_t n){
    SmallString<sz> res{};
    for (char c: t){
        if (c == sym){
            auto [arr, count] = ztoarr<num_c_max>(n);
            for (size_t i = 0; i < count; ++i) res.push(arr[i]);
        } else res.push(c);
    }
    return res;
}

template <std::integral T>
size_t popcount(T val){
    size_t n = 0;
    while(val) {val &= val - 1; ++n;}
    return n;
}

template <std::integral T>
T mod(T t, size_t m){
    return ((t % m) + m) % m;
}

unsigned to_bcd(unsigned val){
    unsigned res{};
    unsigned m = 1;
    while(val){
        res += (m * (val & 0xf));
        m *= 10;
        val >>= 4;
    }
    return res;
}

unsigned from_bcd(unsigned val){
    unsigned res{};
    unsigned nibble_i = 0;
    while(val){
        res |= ((val % 10) << (4 * nibble_i++));
        val /= 10;
    }
    return res;
}

template <std::integral T>
T rotl(T val, size_t count, size_t bits = sizeof(T) * CHAR_BIT){
    while(count){
        bool b = (val >> (bits-1)) & 1;
        val <<= 1;
        val &= ((1 << bits) - 1);
        val &= (~1 & ((1 << bits) - 1));
        val |= b;
    }
    return val;
}

template <std::integral T>
T rotr(T val, size_t count, size_t bits = sizeof(T) * CHAR_BIT){
    while(count){
        bool b = val & 1;
        val >>= 1;
        val |= (b << (bits-1));
    }
    return val;
}

template <typename T, size_t cap>
struct SimpleDequeT{
    T arr[cap]{};
    size_t sz{};

    void push_back(T val){
        arr[sz++] = val;
    }
    void push_front(T val){
        push_back(val);
        for (size_t i = sz - 1; i != 0; --i)
            std::swap(arr[i], arr[i-1]);
    }

    void pop_front(){
        for (size_t i = 0; i != cap-1; ++i)
            std::swap(arr[i], arr[i+1]);
        pop_back();
    }

    void pop_back(){
        --sz;
    }

    size_t size() const {
        return sz;
    }

    T& front(){
        return arr[0];
    }

    T& back(){
        return arr[sz - 1];
    }

    bool empty() const {
        return !sz;
    }

    void clear(){ sz = 0;}

    auto begin(){
        return arr;
    }

    auto end(){
        arr + sz;
    }
};
#endif