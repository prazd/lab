#ifndef SMD_SIMPLE_HPP
#define SMD_SIMPLE_HPP
#include <array>
#include <fstream>

inline auto read_file(std::string_view path){
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) std::abort();
    std::vector<char> res;
    auto sz = std::filesystem::file_size(path);
    res.resize(sz);
    file.read(res.data(), sz);
    return res;
}

inline void read_file(std::string_view path, std::vector<char>& vec){
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) std::abort();
    auto sz = std::filesystem::file_size(path);
    vec.resize(sz);
    file.read(vec.data(), sz);
}

template <std::integral T>
T reverse_bits(T val){
    size_t r = 0;
    size_t l = (sizeof(T) * CHAR_BIT) - 1;
    bool l_v{};
    bool r_v{};
    while(l > r){
        l_v = val & (1 << l);
        r_v = val & (1 << r);

        // swap
        l_v ^= r_v;
        r_v ^= l_v;
        l_v ^= r_v;

        val &= (~(1 << l));
        val |= (l_v << l);
        val &= (~(1 << r));
        val |= (r_v << r);
        --l;
        ++r;
    }
    return val;
}

template <std::integral T>
constexpr size_t reverse_nibbles(T val){
    size_t nibbles = sizeof(T) * 2;
    size_t r = 0;
    size_t l = nibbles-1;
    while (l > r){
        auto l_v = (val >> (l << 2)) & 0xf;
        auto r_v = (val >> (r << 2)) & 0xf;

        l_v ^= r_v;
        r_v ^= l_v;
        l_v ^= r_v;

        val &= (~(0xf << (l << 2)));
        val |= (l_v << (l << 2));

        val &= (~(0xf << (r << 2)));
        val |= (r_v << (r << 2));

        --l;
        ++r;
    }

    while(val){
        if (val & 0xf) break;
        val >>= 4;
    }
    return val;
}


template <std::integral T>
T be_to_le(T value, size_t sz){
    auto rp = (unsigned char*)(&value);
    auto lp = rp + (sz-1);
    while(rp != lp){
        std::swap(*rp, *lp);
        --lp;
        if (rp == lp) break;
        ++rp;
    }
    return value;
}

template <size_t cap>
struct SmallString {
    char buf[cap]{};
    size_t sz = 0;
    constexpr SmallString() = default;
    constexpr explicit SmallString(std::string_view sv){for (auto c: sv) push(c);}
    constexpr explicit SmallString(unsigned n){
        n = reverse_nibbles(n);
        while(n){
            auto val = n & 0xf;
            push(val < 10 ? '0' + val : 'a' + (val - 10));
            n >>= 4;
        }
    }
    constexpr void reset(){sz = 0;}
    constexpr void push(char c){buf[sz++] = c;}
    constexpr void push_lowercase(char c){buf[sz++] = c + 0x20;}
    constexpr void push(std::string_view sv){for (auto c: sv) push(c);}
    constexpr auto begin(){return buf;}
    constexpr auto end(){return buf + sz;}

    constexpr std::string_view to_sv() const{
        return std::string_view(buf, sz);
    }

    constexpr auto operator+(const SmallString& rhs) const {
        SmallString<cap> res(*this);
        res += rhs;
        return res;
    }

    constexpr SmallString& operator+=(const SmallString& rhs){
        for (size_t i = 0; i < rhs.sz; ++i) push(rhs.buf[i]);
        return *this;
    }
};

template <size_t sz>
auto ztoarr(size_t val){
    // TODO: use tuple
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

inline unsigned to_bcd(unsigned val){
    unsigned res{};
    unsigned m = 1;
    while(val){
        res += (m * (val & 0xf));
        m *= 10;
        val >>= 4;
    }
    return res;
}

inline unsigned from_bcd(unsigned val){
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