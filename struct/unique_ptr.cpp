#include <iostream>
#include <memory>
#include <utility>

template <typename T, typename Deleter = decltype([](T* t){std::cout << "DefaultDeleter\n";delete t;})>
struct unique_ptr{
    T* ptr;
    [[no_unique_address]] Deleter del;
    unique_ptr(T* ptr): ptr(ptr){}
    unique_ptr(T* ptr, Deleter d): ptr(ptr), del(d){}
    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;
    unique_ptr(unique_ptr&& rhs): ptr(std::exchange(rhs.ptr, nullptr)), del(std::move(rhs.del)){}
    unique_ptr& operator=(unique_ptr&& rhs) noexcept {
        std::swap(rhs.ptr, ptr);
        std::swap(rhs.del, del);
        return *this;
    }
    T* release() {return std::exchange(ptr, nullptr);}
    void reset(T* rhs = nullptr){if(ptr) del(ptr); ptr = rhs;}
    T* get() const noexcept {return ptr;}
    T& operator*()const noexcept {return *ptr;}
    explicit operator bool()const noexcept {return ptr;}
    ~unique_ptr(){
        if (ptr) del(ptr);
    }
};

template <typename T, typename ... Args>
auto make_unique(Args&& ... args) {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T, typename Deleter = decltype([](std::decay_t<T>* ptr){std::cout << "MakeUniqueCustomDeleterDefault\n"; delete ptr;})>
auto make_unique_custom_del(T&& value) {
    return unique_ptr<std::decay_t<T>, Deleter>(new std::decay_t<T>(std::forward<T>(value)));
}

template <typename T>
auto make_unique_custom_del(T&& value, void(*del)(std::decay_t<T>*)) {
    return unique_ptr<std::decay_t<T>, void(*)(std::decay_t<T>*)>(new std::decay_t<T>(std::forward<T>(value)), del);
}

void Delete(int* d) {
    std::cout << "DeleteFunc\n";
    delete d;
}

int main() {
    auto ptr = unique_ptr<int, decltype([](int* ptr){std::cout << "DeleterLambda\n"; delete ptr;})>(new int(10));
    auto ptr2 = make_unique<int>(20);
    auto ptr3 = unique_ptr<int, void(*)(int*)>(new int(30), Delete);
    
    
    auto ptr4 = make_unique_custom_del(40);
    auto ptr5 = make_unique_custom_del(50, Delete);
    auto ptr6 = make_unique_custom_del<int, decltype([](int* ptr){std::cout << "MakeUniqueCustomDeleter\n"; delete ptr;})>(60);
}