#include <iostream>
#include <cassert>
#include <tuple>
#include <algorithm>
#if 1
template <typename T, size_t sz>
struct CircleStack { 
    alignas(T) unsigned char arr[sizeof(T) * sz]{};   
    size_t current = 0;
    unsigned char* ptr = arr;
    void els_copy(const CircleStack& rhs){
        ptr += rhs.ptr - rhs.arr;
        size_t current_tmp = rhs.current;
        auto ptr_rhs = rhs.ptr;
        auto ptr_cur = ptr;
        while(current_tmp){
            ptr_cur = ptr_cur + ((ptr_cur != arr) ? ptrdiff_t(-1 * sizeof(T)) : ptrdiff_t(sizeof(T) * (sz-1)));
            ptr_rhs = ptr_rhs + ((ptr_rhs != rhs.arr) ? ptrdiff_t(-1 * sizeof(T)) : ptrdiff_t(sizeof(T) * (sz-1)));
            new (ptr_cur) T(*std::launder(reinterpret_cast<T*>(ptr_rhs)));
            --current_tmp;
        }
        current = rhs.current;
    }
    void els_move(CircleStack&& rhs){
        ptr += rhs.ptr - rhs.arr;
        size_t current_tmp = rhs.current;
        auto ptr_rhs = rhs.ptr;
        auto ptr_cur = ptr;
        while(current_tmp){
            ptr_cur = ptr_cur + ((ptr_cur != arr) ? ptrdiff_t(-1 * sizeof(T)) : ptrdiff_t(sizeof(T) * (sz-1)));
            ptr_rhs = ptr_rhs + ((ptr_rhs != rhs.arr) ? ptrdiff_t(-1 * sizeof(T)) : ptrdiff_t(sizeof(T) * (sz-1)));
            new (ptr_cur) T(std::move(
                *std::launder(reinterpret_cast<T*>(ptr_rhs))));
            --current_tmp;
            std::launder(reinterpret_cast<T*>(ptr_rhs))->~T();
        }
        current = rhs.current;
        rhs.current = 0;
        rhs.ptr = rhs.arr;
    }

    CircleStack() = default;
    CircleStack(const CircleStack& rhs){
        els_copy(rhs);
    }
    CircleStack(CircleStack&& rhs){
        els_move(std::move(rhs));
    }

    CircleStack& operator=(const CircleStack& rhs){
        if (&rhs != this) {
            while(!empty()) pop();
            ptr = arr;
            els_copy(rhs);
        }
        return *this;
    }

    CircleStack& operator=(CircleStack&& rhs){
         if (&rhs != this) {
            while(!empty()) pop();
            ptr = arr;
            els_move(std::move(rhs));
        }
        return *this;
    }

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
    void els_copy(const CircleStack& rhs){
        ptr += rhs.ptr - rhs.arr;
        size_t current_tmp = rhs.current;
        auto ptr_rhs = rhs.ptr;
        auto ptr_cur = ptr;
        while(current_tmp){
            ptr_cur += (ptr_cur != arr) ? -1 : sz-1;
            ptr_rhs += (ptr_rhs != rhs.arr) ? -1 : sz-1;
            *ptr_cur = *ptr_rhs;
            --current_tmp;
        }
        current = rhs.current;
    }
    void els_move(CircleStack&& rhs){
        ptr += rhs.ptr - rhs.arr;
        size_t current_tmp = rhs.current;
        auto ptr_rhs = rhs.ptr;
        auto ptr_cur = ptr;
        while(current_tmp){
            ptr_cur += (ptr_cur != arr) ? -1 : sz-1;
            ptr_rhs += (ptr_rhs != rhs.arr) ? -1 : sz-1;
            *ptr_cur = std::move(*ptr_rhs);
            --current_tmp;
        }
        current = rhs.current;
        rhs.current = 0;
        rhs.ptr = rhs.arr;
    }
    
    CircleStack() = default;
    CircleStack(const CircleStack& rhs){
        els_copy(rhs);
    }
    CircleStack(CircleStack&& rhs){
        els_move(std::move(rhs));
    }

    CircleStack& operator=(const CircleStack& rhs){
        if (&rhs != this) {
            while(!empty()) pop();
            ptr = arr;
            els_copy(rhs);
        }
        return *this;
    }

    CircleStack& operator=(CircleStack&& rhs){
         if (&rhs != this) {
            while(!empty()) pop();
            ptr = arr;
            els_move(std::move(rhs));
        }
        return *this;
    }
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

void copy_and_move_test(){
    CircleStack<int, 3> cb;
    cb.push(1);cb.push(2);cb.push(3);

    CircleStack<int, 3> cb2;
    cb2.push(4);cb2.push(5);cb2.push(6);

    CircleStack<int, 3> cb3;
    cb3.push(7);cb3.push(8);cb3.push(9);

    CircleStack<int, 3> cb_cp = cb;
    assert(cb_cp.top() == 3 && (unsigned char*)cb_cp.ptr == (unsigned char*)cb_cp.arr); cb_cp.pop();
    assert(cb_cp.top() == 2 && (unsigned char*)cb_cp.ptr == (unsigned char*)cb_cp.arr+sizeof(int)*2); cb_cp.pop();
    assert(cb_cp.top() == 1 && (unsigned char*)cb_cp.ptr == (unsigned char*)cb_cp.arr + sizeof(int)); cb_cp.pop();

    cb_cp = cb;
    assert(cb_cp.top() == 3 && (unsigned char*)cb_cp.ptr == (unsigned char*)cb_cp.arr); cb_cp.pop();
    assert(cb_cp.top() == 2 && (unsigned char*)cb_cp.ptr == (unsigned char*)cb_cp.arr+sizeof(int)*2); cb_cp.pop();
    assert(cb_cp.top() == 1 && (unsigned char*)cb_cp.ptr == (unsigned char*)cb_cp.arr + sizeof(int)); cb_cp.pop();

    cb_cp = cb;
    cb_cp = cb2;
    assert(cb_cp.top() == 6 && (unsigned char*)cb_cp.ptr == (unsigned char*)cb_cp.arr); cb_cp.pop();
    assert(cb_cp.top() == 5 && (unsigned char*)cb_cp.ptr == (unsigned char*)cb_cp.arr+sizeof(int)*2); cb_cp.pop();
    assert(cb_cp.top() == 4 && (unsigned char*)cb_cp.ptr == (unsigned char*)cb_cp.arr + sizeof(int)); cb_cp.pop();
    

    CircleStack<int, 3> cb_cp2 = std::move(cb);
    assert(cb_cp2.top() == 3 && (unsigned char*)cb_cp2.ptr == (unsigned char*)cb_cp2.arr); cb_cp2.pop();
    assert(cb_cp2.top() == 2 && (unsigned char*)cb_cp2.ptr == (unsigned char*)cb_cp2.arr+sizeof(int)*2); cb_cp2.pop();
    assert(cb_cp2.top() == 1 && (unsigned char*)cb_cp2.ptr == (unsigned char*)cb_cp2.arr + sizeof(int)); cb_cp2.pop();

    cb_cp2 = std::move(cb2);
    cb_cp2 = std::move(cb3);

    assert(cb_cp2.top() == 9 && (unsigned char*)cb_cp2.ptr == (unsigned char*)cb_cp2.arr); cb_cp2.pop();
    assert(cb_cp2.top() == 8 && (unsigned char*)cb_cp2.ptr == (unsigned char*)cb_cp2.arr+sizeof(int)*2); cb_cp2.pop();
    assert(cb_cp2.top() == 7 && (unsigned char*)cb_cp2.ptr == (unsigned char*)cb_cp2.arr + sizeof(int)); cb_cp2.pop();
}


int main(){
    push_test();
    pop_test();
    copy_and_move_test();
}