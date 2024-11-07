#include <iostream>
#include <cassert>

class any {
    struct Base {
        virtual Base& get_value() = 0;
        virtual ~Base(){};
    };

    Base* base;

    template <typename T>
    struct Derived: public Base {
        T value;
        Derived(const T& value): value(value) {}
        Derived(T&& value): value(std::move(value)) {}
        Derived& get_value() override {
            return *this;
        }
        operator T() {
            return value;
        }
        ~Derived() = default;
    };

public:
    template <typename T>
    any(const T& value): base(new Derived<T>(value)) {}

    template <typename T>
    any& operator=(const T& value) {
        if (sizeof(value) == sizeof(*base)) {
            base->~Base();
            new (base) Derived<T>(value);
            return *this;
        }
        delete base;
        base = new Derived<T>(value);
        return *this;
    }

    template <typename T>
    T get_value() const {
        auto derived = dynamic_cast<any::Derived<T>&>(base->get_value());
        if (!derived) {
            throw std::bad_cast();
        }
        return static_cast<T>(derived);
    }

    ~any() {
       delete base;
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

int main() {
    any x = 10;
    assert(any_cast<int>(x) == 10);
    x = 123.2f;
    assert(any_cast<float>(x) == 123.2f);
    x = 8UL;
    assert(x.get_value<unsigned long>() == 8UL);
    x = 42.0;
    assert(x.get_value<double>() == 42.0);
}