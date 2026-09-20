#ifndef SMD_SIMPLE_HPP
#define SMD_SIMPLE_HPP

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

    void clear(){cur_sz = 0;}

    auto begin(){
        return arr;
    }

    auto end(){
        arr + cur_sz;
    }
};
#endif