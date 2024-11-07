#include <iostream>
#include <cassert>
#include <memory>

class any {
    struct Base {
        virtual ~Base(){};
    };

    char buffer[16];
    Base* stack_base;
    std::unique_ptr<Base> heap_base;
    
    template <typename T>
    struct Derived: public Base {
        T value;
        Derived(const T& value): value(value) {}
        Derived(T&& value): value(std::move(value)) {}
        ~Derived() = default;
    };

public:
    template <typename T>
    void stack_alloc(const T& value) {
        new (buffer) Derived<T>(value);
        stack_base = reinterpret_cast<Derived<T>*>(buffer);
    }

    template <typename T>
    any(const T& value): stack_base(nullptr), heap_base(nullptr) {
        if (sizeof(value) <= sizeof(buffer)) {
            stack_alloc<T>(value);
        } else {
            heap_base = std::make_unique<Derived<T>>(value);
        }
    }

    template <typename T>
    any& operator=(const T& value) {
        Base* current;
        if (heap_base) {
            current = heap_base.get();
        } else {
            current = stack_base;
        }
        if (auto derived = dynamic_cast<Derived<T>*>(current)) {
            derived->value = value;
            return *this;
        } else if (sizeof(value) <= sizeof(buffer)){
            if (heap_base){
                heap_base = nullptr;
            } else if (stack_base) {
                stack_base->~Base();
            }
            stack_alloc<T>(value);
            return *this;
        }
        stack_base->~Base();
        heap_base = std::make_unique<Derived<T>>(value);
        return *this;
    }

    template <typename T>
    T get_value() const {
        Base* current;
        if (heap_base) {
            current = heap_base.get();
        } else {
            current = stack_base;
        }
        auto derived = dynamic_cast<any::Derived<T>*>(current);
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
}