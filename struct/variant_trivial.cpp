#include <iostream>
#include <variant>
#include <cstring>
#include <memory>
#include <cassert>

template <typename ... T>
consteval bool is_all_trivial() {
    return (std::is_trivial_v<T> && ...);
}
template <typename Tuple, size_t ... I>
consteval auto find_max_sz_impl(std::index_sequence<I...>) {
    size_t res = 0;
    ((res = std::max(sizeof(typename std::tuple_element<I, Tuple>::type), res)), ...);
    return res;
}

template <typename ... Ts>
consteval auto find_max_sz() {
    return find_max_sz_impl<std::tuple<Ts...>>(std::index_sequence_for<Ts...>{});
}

template <typename Tuple, size_t ... I>
consteval auto find_max_align_impl(std::index_sequence<I...>) {
    size_t res = 0;
    ((res = std::max(alignof(typename std::tuple_element<I, Tuple>::type), res)), ...);
    return res;
}

template <typename ... Ts>
consteval auto find_max_align() {
    return find_max_align_impl<std::tuple<Ts...>>(std::index_sequence_for<Ts...>{});
}

template <typename Tuple, typename Check, size_t ... I>
consteval auto find_index_impl(std::index_sequence<I...>) {
    size_t res = 0;
    ((res = std::is_same_v<Check, typename std::tuple_element<I, Tuple>::type> ? I : res), ...);
    return res;
}

template <typename Check, typename ... Ts>
consteval auto find_index() {
    return find_index_impl<std::tuple<Ts...>, Check>(std::index_sequence_for<Ts...>{});
}

template <size_t I, typename ... Ts>
struct type_by_index {
    using type = std::decay_t<decltype(std::get<I>(std::tuple<Ts...>{}))>;
};

template <typename Check, typename ... T>
consteval bool is_one_of() {
    return (std::is_same<T, Check>() || ...);
}

template <typename ... Ts>
class VariantTrivial {
    static_assert(is_all_trivial<Ts...>());
    alignas(find_max_align<Ts...>()) char buffer[find_max_sz<Ts...>()]{};
    unsigned char index{};
public:
    VariantTrivial(): index(0) {}

    template <typename T>
    VariantTrivial(T value) {
        static_assert(is_one_of<T, Ts...>());
        new (buffer) T(value);
        index = find_index<T, Ts...>();
    }

    void clear() {
        std::memset(buffer, 0, sizeof(buffer));
        index = 0;
    }

    unsigned char get_index() const {
        return index;
    }

    template <typename T>
    VariantTrivial& operator=(T value){
        static_assert(is_one_of<T, Ts...>());
        clear();
        index = find_index<T, Ts...>();
        new (buffer) T(value);
        return *this;
    }

    template <size_t n>
    auto& get() {
        assert(get_index() == n);
        return *reinterpret_cast<typename type_by_index<n, Ts...>::type*>(buffer);
    }
};

int main() {
    VariantTrivial<int, char, long> v;
    v = 10;
    assert(v.get<0>() == 10);
    char z = 20;
    v = z;
    assert(v.get<1>() == 20);
    long f = 30;
    v = f;
    assert(v.get<2>() == 30);
    auto& current = v.get<2>();
    current = 50;
    assert(v.get<2>() == 50);
}