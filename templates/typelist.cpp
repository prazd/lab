#include <iostream>
#include <type_traits>

template <typename ... T>
struct TypeList {
    // Length
    static constexpr inline size_t length = sizeof...(T);

    // At
    template <size_t I, typename Head, typename ... Tail>
    struct type_at {
        using type = type_at<I-1, Tail...>::type;
    };
    template <typename Head, typename ... Tail>
    struct type_at<0, Head, Tail...> {
        using type = Head;
    };
    template <size_t n>
    using at = typename type_at<n, T...>::type;


    // Push/Pop
    template <typename U>
    using push_front = TypeList<U, T...>;

    template <typename U>
    using push_back = TypeList<T..., U>;

    template <typename Head, typename ... Tail>
    struct type_pack {
        using type = TypeList<Tail...>;
    };
    using pop_front = type_pack<T...>::type;


    // Contains
    template <typename U, typename ... Tail>
    struct find{};

    template <typename U, typename Head, typename ... Tail>
    struct find<U, Head, Tail...>{
        static auto constexpr res = find<U, Tail...>::res;
    };

    template <typename U, typename ... Tail>
    struct find<U, U, Tail...> {
        static auto constexpr res = std::true_type{};
    };

    template <typename U>
    struct find<U>{
        static auto constexpr res = std::false_type{};
    };
    
    template <typename U>
    constexpr static auto contains = find<U, T...>::res;

    // 0 0 1 1 0 0

    // Filter
    template <template<typename> class Predicat, typename Current, typename... Ts>
    struct f;

    template <template<typename> class Predicat, typename Current, typename Head, typename ... Ts>
    struct f <Predicat, Current, Head, Ts...>{
        using res = std::conditional_t<
        Predicat<Head>::value,
        typename f<Predicat, typename Current::template push_back<Head>, Ts...>::res,
        typename f<Predicat, Current, Ts...>::res
        >;
    };

    template <template<typename> class Predicat, typename Current>
    struct f<Predicat, Current> {
        using res = Current;
    };

    template <template<typename> class Predicat>
    using filter = f<Predicat, TypeList<>, T...>::res;
};

template<>
struct TypeList<>{
    template <typename U>
    using push_back = TypeList<U>;
};


// Predicate
template<typename T>
struct is_integral : std::is_integral<T> {};

int main() {
    using MyList = TypeList<int, char, float, double, short>;
    static_assert(MyList::length == 5);
    static_assert(std::is_same_v<MyList::at<0>, int>);
    static_assert(std::is_same_v<MyList::at<2>, float>);

    using WithBool = MyList::push_front<bool>;
    static_assert(WithBool::length == 6);
    static_assert(std::is_same_v<WithBool::at<0>, bool>);

    using Popped = WithBool::pop_front;
    static_assert(std::is_same_v<Popped, MyList>);

    static_assert(MyList::contains<int>);
    static_assert(!MyList::contains<bool>);

    using OnlyIntegral = MyList::filter<is_integral>;
    static_assert(std::is_same_v<OnlyIntegral, TypeList<int, char, short>>);
}