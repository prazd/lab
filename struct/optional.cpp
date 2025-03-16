#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <optional>

template <typename T, bool = std::is_trivial_v<T>>
class optional;

template <typename T>
class optional<T, false>{
    
    alignas(alignof(T)) std::byte buffer[sizeof(T)];
    bool has_val = false;

    template <typename U>
    void construct_U(U&& value){
        std::construct_at(reinterpret_cast<T*>(buffer), std::forward<U>(value));
        has_val = true;
    }

    void move_construct(optional<T>&& value) {
        std::construct_at(reinterpret_cast<T*>(buffer), std::move(*reinterpret_cast<T*>(value.buffer)));
        has_val = true;
        value.has_val = false;
    }
    
    void copy_construct(const optional<T>& value) {
        std::construct_at(reinterpret_cast<T*>(buffer), *reinterpret_cast<const T*>(value.buffer));
        has_val = true;
    }

    void copy(const optional<T>& value) {
        *reinterpret_cast<T*>(buffer) = *reinterpret_cast<const T*>(value.buffer);
    }

    void move_O(optional<T>&& value) {
        *reinterpret_cast<T*>(buffer) = std::move(*reinterpret_cast<T*>(value.buffer));
    }

public:
    optional() = default;

    template <typename U,
        std::enable_if_t<
        !std::is_same_v<std::decay_t<U>, optional<T>> &&
        !std::is_same_v<std::remove_cvref_t<U>, optional<T>>, int> = 0>
    optional(U&& value){
        construct_U(std::forward<U>(value));
    }

    template <typename U,
        std::enable_if_t<
        !std::is_same_v<std::decay_t<U>, optional<T>> &&
        !std::is_same_v<std::remove_cvref_t<U>, optional<T>>, int> = 0>
    optional<T>& operator=(U&& value) {
        if (has_val) {
            auto& ref = *std::launder(reinterpret_cast<T*>(buffer));
            ref = std::forward<U>(value);
        } else {
            construct_U(std::forward<U>(value));
        }
        return *this;
    }

    optional(const optional<T>& rhs) requires std::copy_constructible<T>{
        if (rhs.has_val)
            copy_construct(rhs);
    };

    optional(optional<T>&& rhs) noexcept requires std::move_constructible<T>{
        if (rhs.has_val)
            move_construct(std::move(rhs));
    };

    optional<T>& operator=(optional<T>&& rhs) noexcept {
        if (&rhs != this) {
            if (has_val && rhs.has_val)
                move_O(std::move(rhs));
            else if (has_val && !rhs.has_val) 
                reset();
            else if (!has_val && rhs.has_val)
                move_construct(std::move(rhs));
        }
        return *this;
    };


    optional<T>& operator=(const optional<T>& rhs) {
        if (&rhs != this) {
            if (rhs.has_val && has_val) 
                copy(rhs);
            else if (rhs.has_val && !has_val) 
                copy_construct(rhs);
            else if (!rhs.has_val && has_val)
                reset();
        }
        return *this;
    }

    void reset() {
        if (has_val) {
            std::destroy_at(reinterpret_cast<T*>(buffer));
            has_val = false;
        };
    }

    bool has_value() const {
        return has_val;
    }

    T& operator*() {
        return *std::launder(reinterpret_cast<T*>(buffer));
    }

    T* operator->() {
        return std::launder(reinterpret_cast<T*>(buffer));
    }

    T& value() {
        if (!has_val)
            throw std::bad_optional_access();
        return *std::launder(reinterpret_cast<T*>(buffer));
    }
    
    ~optional(){reset();};
};

template <typename T>
class optional<T, true>{
        T v;
        bool has_val = false;
public:
        optional() = default;
        optional(T v): v(v), has_val(true){}
    
        optional& operator=(T value) {
            v = value;
            has_val = true;
            return *this;
        }

        optional(const optional&) = default;
        optional(optional&&) noexcept = default;
        optional& operator=(const optional& rhs) = default;
        optional& operator=(optional&& rhs) noexcept = default;

        void reset() {
            has_val = false;
        }
    
        bool has_value() const {
            return has_val;
        }
        T& operator*() {
            return v;
        }
        T* operator->() {
            return &v;
        }
        T& value() {
            if (!has_val)
                throw std::bad_optional_access();
            return v;
        }
};


int main() {
    optional<std::string> s = "123";
    assert(s.value() == "123");
    s = "456";
    assert(s.value() == "456");
    s = {};
    assert(!s.has_value());

    auto get_val = [](){return optional<long>{};};
    auto get_val2 = [](){return optional<long>{100};};

    optional<long> x = get_val();
    assert(!x.has_value());
    x = get_val2();
    assert(x.has_value());
    assert(*x == 100);
    assert(sizeof(optional<long>) == sizeof(std::optional<long>));
    x.reset();
    assert(x.has_value() == false);

    std::optional<std::vector<int>> v1;
    optional<std::vector<int>> v2;
    v1 = std::vector<int>{1,2,3};
    v2 = std::vector<int>{1,2,3};
    optional<std::vector<int>> v3 = v2;
    assert(v1.value() == v2.value() && v2.value() == v3.value());
    assert(sizeof(v1) == sizeof(v2));

    v1.reset();
    v2.reset();
    assert(v1.has_value() == v2.has_value());

    optional<std::vector<int>> v4;
    v4 = std::vector<int>{1,2,3};
    optional<std::vector<int>> v5 = std::move(v4);
    assert(v5.value()[0] == 1);
    assert(v5.value()[1] == 2);
    assert(v5.value()[2] == 3);
    optional<std::vector<int>> v6;
    v6 = std::move(v5);
    assert(v6.value()[0] == 1);
    assert(v6.value()[1] == 2);
    assert(v6.value()[2] == 3);


    optional<std::vector<int>> v7 = std::vector<int>{1,2,3,4};
    optional<std::vector<int>> v8 = std::vector<int>{4,5,6,7,8};
    v7 = v8;
    assert(v7.value() == v8.value());
    optional<std::vector<int>> v9 = std::vector<int>{10,10,10,10,10};
    v7 = std::move(v9);
    for(auto el: *v7)
        assert(el == 10);
    assert(!v7->empty());
}