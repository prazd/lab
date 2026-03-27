#ifndef NES_SIMPLEDEQUE_H
#define NES_SIMPLEDEQUE_H
#include <iostream>
template <typename T, size_t sz>
struct SimpleDeque {
    int current_sz = 0;
    alignas(T) unsigned char buffer[sz * sizeof(T)]{};
    SimpleDeque() = default;

    struct Iter {
        unsigned char* current;
        Iter& operator++(){
            current = current = reinterpret_cast<unsigned char*>(reinterpret_cast<T*>(current) + 1);
            return *this;
        }
        Iter& operator--(){
            current = current = reinterpret_cast<unsigned char*>(reinterpret_cast<T*>(current) + 1);
            return *this;
        }
        Iter operator+(int i){
            return Iter{reinterpret_cast<unsigned char*>(reinterpret_cast<T*>(current) + i)};
        }
        T& operator*(){
            return *reinterpret_cast<T*>(current);
        }
        unsigned char* data(){
            return current;
        }
        bool operator!=(const Iter& iter){
            return iter.current != current;
        }
    };

    auto begin(){
        return Iter{buffer};
    }

    auto end(){
        return Iter{buffer + sizeof(T) * current_sz};
    }

    void erase(Iter begin, Iter end){
        current_sz = (begin.data() - buffer)/sizeof(T);
        for (; begin != end; ++begin)
            reinterpret_cast<T*>(begin.data())->~T();
    }

    template <typename U>
    void push_back(U&& value){
        new (buffer + current_sz++ * sizeof(T)) T(std::forward<U>(value));
    }

    template <typename U>
    void push_front(U&& value){
        push_back(std::forward<U>(value));
        for (int i = current_sz-2; i >= 0; --i)
            std::swap(this->operator[](i), this->operator[](i+1));
    }

    void pop_back(){
        if (current_sz) {
            reinterpret_cast<T*>(buffer + (current_sz-1) * sizeof(T))->~T();
            --current_sz;
        }
    }

    void pop_front(){
        for (int i = 0; i <= current_sz-2; ++i)
            std::swap(this->operator[](i), this->operator[](i+1));
        pop_back();
    }

    T& operator[](size_t index){
        return  *reinterpret_cast<T*>(buffer + index * sizeof(T));
    }
    const T& operator[](size_t index)const{
        return  *reinterpret_cast<const T*>(buffer + index * sizeof(T));
    }
    size_t size() const {
        return current_sz;
    }
    void reset(){
        for (int i = 0; i < current_sz; ++i) reinterpret_cast<T*>(buffer + i * sizeof(T))->~T();
#if DEBUG_MODE
        std::memset(buffer, 0, sizeof(T) * sz);
#endif
        current_sz = 0;
    }
    ~SimpleDeque(){
        for (int i = 0; i < current_sz; ++i)  reinterpret_cast<T*>(buffer + i * sizeof(T))->~T();
    }
};
#endif
