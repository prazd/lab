#include <iostream>
#include <cassert>
#include <tuple>

#if 1
template <typename T, int sz>
struct CircleStack {
    alignas(T) unsigned char arr[sizeof(T) * sz]{};   
    int current = 0;
    unsigned char* ptr = arr;

    template <typename U>
    void push(U&& value){
        if (current == sz) std::launder(reinterpret_cast<T*>(ptr))->~T();
        new (ptr) T(std::forward<U>(value));
        current = std::min(current + 1, sz);
        ptr += (ptr + sizeof(T) == arr + sz * sizeof(T)) ? ptrdiff_t(-1 * (sizeof(T) * (sz-1))) : ptrdiff_t(sizeof(T));
    }
    
    void pop(){
        current -= ((current - 1) < current) * 1;
        ptr += (ptr != arr) ? ptrdiff_t(-1 * sizeof(T)) : ptrdiff_t(sizeof(T) * (sz-1));
        std::launder(reinterpret_cast<T*>(ptr))->~T();
    }
    
    bool empty() const {
        return !current;
    }

    T& top() {
        return *std::launder(reinterpret_cast<T*>(ptr + ((ptr != arr) ? ptrdiff_t(-1 * sizeof(T)) : ptrdiff_t(sizeof(T) * (sz-1)))));
    }
    ~CircleStack(){
        while(!empty()) pop();
    }
};
#else
template <typename T, int sz>
struct CircleStack {
    T arr[sz]{};
    int current = 0;
    T* ptr = arr;
    void push(const T& value){
        *ptr = value;
        current = std::min(current + 1, sz);
        ptr += (ptr + 1 == arr + sz) ? (-1 * (sz-1)) : 1;
    }
    void pop(){
        current -= ((current - 1) < current) * 1;
        ptr += (ptr != arr) ? -1 : sz-1;
    }
    bool empty() const {
        return !current;
    }
    T& top() {
        return *(ptr + ((ptr != arr) ? -1 : sz-1));
    }
    void print(){
        for (int i = 0; i < sz; ++i)
            std::cout << arr[i] << ' ';
        std::cout << '\n';
    }
};
#endif


void push_test() {
    CircleStack<int, 3> cb;
    std::tuple<int,void*,int> push_test_cases[] = {
        { 1, (unsigned char*)cb.arr + sizeof(int), 1},
        { 2, (unsigned char*)cb.arr + 2 * sizeof(int), 2},
        { 3, (unsigned char*)cb.arr, 3},
        { 4, (unsigned char*)cb.arr + sizeof(int), 4},
        { 5, (unsigned char*)cb.arr + 2 * sizeof(int), 5},
        { 6, (unsigned char*)cb.arr, 6},
    };
    for (auto& [arg, ptr, top_value]: push_test_cases){
        cb.push(arg);
        assert((unsigned char*)cb.ptr == ptr);
        assert(cb.top() == top_value);
    }
}


void pop_test() {
    CircleStack<int, 3> cb;
    cb.push(1);
    cb.push(2);
    cb.push(3);
    assert(cb.top() == 3 && (unsigned char*)cb.ptr == (unsigned char*)cb.arr); cb.pop();
    assert(cb.top() == 2 && (unsigned char*)cb.ptr == (unsigned char*)cb.arr + 2 * sizeof(int)); cb.pop();
    assert(cb.top() == 1 && (unsigned char*)cb.ptr == (unsigned char*)cb.arr + sizeof(int)); cb.pop();

    cb.push(1);
    cb.push(2);
    cb.push(3);
    cb.push(4);
    assert(cb.top() == 4 && (unsigned char*)cb.ptr == (unsigned char*)cb.arr+sizeof(int)); cb.pop();
    assert(cb.top() == 3 && (unsigned char*)cb.ptr == (unsigned char*)cb.arr); cb.pop();
    assert(cb.top() == 2 && (unsigned char*)cb.ptr == (unsigned char*)cb.arr+sizeof(int)*2); cb.pop();

    cb.push(1);
    cb.push(2);
    cb.push(3);
    assert(cb.top() == 3 && (unsigned char*)cb.ptr == (unsigned char*)cb.arr + sizeof(int)); cb.pop();
    assert(cb.top() == 2 && (unsigned char*)cb.ptr == (unsigned char*)cb.arr); cb.pop();
    assert(cb.top() == 1 && (unsigned char*)cb.ptr == (unsigned char*)cb.arr+sizeof(int)*2); cb.pop();

    const int res[3]{8,7,6};
    cb.push(4);
    cb.push(5);
    cb.push(6);
    cb.push(7);
    cb.push(8);
    int cur = 0;
    while(!cb.empty()){
        assert(res[cur++] == cb.top()); cb.pop();
    }

    cb.push(1);
    cb.push(2);
    cb.push(3);
    assert(cb.top() == 3 && (unsigned char*)cb.ptr == (unsigned char*)cb.arr); cb.pop();
    assert(cb.top() == 2 && (unsigned char*)cb.ptr == (unsigned char*)cb.arr+sizeof(int)*2); cb.pop();
    assert(cb.top() == 1 && (unsigned char*)cb.ptr == (unsigned char*)cb.arr + sizeof(int)); cb.pop();
}

int main(){
    push_test();
    pop_test();
}