#ifndef SMD_SIMPLE_HPP
#define SMD_SIMPLE_HPP

template <size_t sz>
struct SmallString {
    char buf[sz]{};
    auto begin(){return buf;}
    auto end(){return buf + sz;}
    std::string_view to_sv() {
        return std::string_view(buf, sz);
    }
};

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

template <typename T, size_t sz>
struct SimpleDequeT{
    T arr[sz]{};
    size_t cur_sz{};

    void push_back(T val){
        arr[cur_sz++] = val;
    }

    void push_front(T val){
        push_back(val);
        for (size_t i = cur_sz-1; i != 0; --i) 
            std::swap(arr[i], arr[i-1]);
    }

    void pop_front(){
        for (size_t i = 0; i != sz-1; ++i) 
            std::swap(arr[i], arr[i+1]);
        pop_back();
    }

    void pop_back(){
        --cur_sz;
    }

    size_t size() const {
        return cur_sz;
    }

    T& front(){
        return arr[0];
    }

    T& back(){
        return arr[cur_sz-1];
    }

    bool empty() const {
        return !cur_sz;
    }

    void clear(){cur_sz = 0;}

    auto begin(){
        return arr;
    }

    auto end(){
        arr + cur_sz;
    }
};
#endif