#include <iostream>
#include <cassert>
#include <memory>

class any {
public:    
    struct Base {
        virtual ~Base(){};
    };
    static inline constexpr size_t small_type_sz_limit = 8;

    // 8 bytes - vtable ptr 
    // 8 bytes - small type size limit
    char buffer[small_type_sz_limit + sizeof(Base*)];

    static void stack_deleter(Base* value) {
        value->~Base();
    }

    static void heap_deleter(Base* value) {
        delete value;
    }
    
    using deleter_func = void(*)(Base*);
    std::unique_ptr<Base, deleter_func> base;
    
    template <typename T>
    struct Derived: public Base {
        T value;
        Derived(const T& value): value(value) {}
        Derived(T&& value): value(std::move(value)) {}
        ~Derived() = default;
    };

    template <typename T>
    Derived<T>* stack_alloc(const T& value) {
        new (buffer) Derived<T>(value);
        return reinterpret_cast<Derived<T>*>(buffer);
    }
    
public:

    template <typename T>
    any(const T& value): base(sizeof(value) <= small_type_sz_limit ? 
    std::unique_ptr<Base, deleter_func>(stack_alloc<T>(value), &stack_deleter) : 
    std::unique_ptr<Base, deleter_func>(new Derived<T>(value), &heap_deleter)) {}

    template <typename T>
    any& operator=(const T& value) {
        if (auto derived = dynamic_cast<Derived<T>*>(base.get())) {
            derived->value = value;
            return *this;
        } else if (sizeof(value) <= small_type_sz_limit){
            base = std::unique_ptr<Base, deleter_func>(stack_alloc<T>(value), &stack_deleter);
            return *this;
        }
        base = std::unique_ptr<Base, deleter_func>(new Derived<T>(value), &heap_deleter);
        return *this;
    }

    template <typename T>
    T get_value() const {
        auto derived = dynamic_cast<any::Derived<T>*>(base.get());
        if (!derived) {
            throw std::bad_cast();
        }
        return derived->value;
    }
};

template <typename T>
T any_cast(const any& rhs) {
    return rhs.get_value<T>();
}

void* operator new(size_t sz) {
    std::cout << "Alloc:" << sz << "\n";
    return malloc(sz);
}

struct Custom {
    long x{10};
    long y{20};
    long z{30};
    long d{40};
};

struct Custom1 {
    long x{10};
    long y{20};
};

int main() {
    any x = 10;
    assert(any_cast<int>(x) == 10);
    x = 30;
    assert(any_cast<int>(x) == 30);
    x = 123.2f;
    assert(any_cast<float>(x) == 123.2f);
    x = 8UL;
    assert(x.get_value<unsigned long>() == 8UL);
    x = 42.0;
    assert(x.get_value<double>() == 42.0);

    x = Custom{};
    assert(x.get_value<Custom>().x == 10);
    assert(x.get_value<Custom>().y == 20);
    assert(x.get_value<Custom>().z == 30);
    assert(x.get_value<Custom>().d == 40);

    x = Custom1{};
    assert(x.get_value<Custom1>().x == 10);
    assert(x.get_value<Custom1>().y == 20);
}