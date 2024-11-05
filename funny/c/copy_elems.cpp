#include <iostream>
#include <tuple>

template <typename T, size_t N>
T* copy_elements(T* (&arr)[N]) {
    T* result = new T[N];
    auto next = result;
    for(size_t i = 0; i < N;++i) 
        result[i] = *(arr[i]);
    return result;
}

struct Base {
    short x;
    short y;
};

std::ostream& operator<<(std::ostream& os, const Base& b) {
    os << "x:" << b.x << " y:" << b.y;
    return os;
}

template <typename... Args>
struct CopyElems{
    std::tuple<Args...>* data;

    template <typename... U>
    CopyElems(U&&... args): data(new std::tuple<Args...>(std::forward<U>(args)...)) {}

    template <typename T>
    decltype(auto) get() const {
        return std::get<T>(*data);
    }

    template <size_t N>
    decltype(auto) get() const {
        return std::get<N>(*data);
    }

    ~CopyElems(){
        delete data;
    }
};

struct Mega {
    long x;
    long y;
};

std::ostream& operator<<(std::ostream& os, const Mega& m) {
    os << "x:" << m.x << " y:" << m.y;
    return os;
}


int main() {
    int x = 10;
    Base b1{10,20};
    Mega m1{20000,3000};

    CopyElems<int, Base, Mega> data(x, b1, m1);

    std::cout << "Data0:" << data.get<0>() << std::endl;
    std::cout << "Data1:" << data.get<1>() << std::endl;
    std::cout << "Data2:" << data.get<2>() << std::endl;
    std::cout << std::endl;
    std::cout << "Data_Int:" << data.get<int>() << std::endl;
    std::cout << "Data_Base:" << data.get<Base>() << std::endl;
    std::cout << "Data_Mega:" << data.get<Mega>() << std::endl;
}