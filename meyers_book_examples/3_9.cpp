#include <iostream>
#include <array>

// В С++1 1 функции constexpr могут содержать не более одной выполнимой инструк­ ции - return.
//  Это выглядит более ограничивающим, чем является на самом деле, по­ скольку для повышения выразительности соnstехрr-функций можно использовать две хитрости.
//  Во-первых, можно применять условный оператор " ? : " вместо инструкции if-else, а во-вторых, вместо циклов можно использовать рекурсию.
//  Таким образом, функция pow может быть реализована следующим образом:

// c++ 11
constexpr int pow11(int base, int exp) noexcept {
    return (exp == 0 ? 1 : base * pow11(base, exp - 1));
}


// В С++ 14 ограничения на соnstехрr-функции существенно слабее, так что становится возможной следующая реализация:
// c++ 14
constexpr int pow14(int base, int exp) noexcept {
    auto result = 1;
    for (int i = 0; i < exp; ++i) result *= base;
    return result;
}

struct Point{
    // Здесь конструктор Point может быть объявлен как constexpr, поскольку, если пере­ данные ему аргументы известны во время компиляции,
    //  значения членов-данных создан­ ного Point также могут быть известны во время компиляции.
    constexpr Point(double x = 0, double y = 0): x(x), y(y) {}
    
    // Аналогично функции доступа xValue и yValue могут быть constexpr, поскольку если они вызываются для объекта Point со значением, 
    // известным во время компиляции (на­ пример, объект constexpr Point), значения членов-данных х и у могут быть известны во время компиляции
    constexpr double x_value() const noexcept {
        return x;
    }
    constexpr double y_value() const noexcept {
        return y;
    }

    // В С++11 два ограничения предотвращают объявление функций-членов Poiпt setX и setY как constexpr. 
    // Во-первых, они модифицируют объект, с которым работают, а в С++11 функции-члены constexpr неявно являются const. 
    // Во-вторых, они имеют возвращаемый тип void, а void не является литеральным типом в С++11. 

    //  void set_x(double new_x) noexcept {
    //     x = new_x;
    //  }
    //  void set_y(double new_y) noexcept {
    //     y = new_y;
    //  }

    // Оба эти ограничения сняты в С++14, так что в С++14 даже функции установки полей Point могут быть объ­ явлены как constexpr:
    constexpr void set_x(double new_x) noexcept {
        x = new_x;
    }
    constexpr void set_y(double new_y) noexcept {
        y = new_y;
    }

private:
    double x;
    double y;    
};


// Это делает возможным написать соnstехрr-функции, которые вызывают функции доступа Point и инициализируют 
// соnstехрr-объекты результатами вызовов этих функций:
constexpr
Point midpoint(const Point& p1, const Point& p2) noexcept {
return {(p1.x_value()+p2.x_value())/2, // Вьзов constexpr
      (p1.y_value()+p2.y_value())/2}; // функции-члена
}

// ...  Оба эти ограничения сняты в С++14, так что в С++14 даже функции установки полей Point могут быть объ­ явлены как constexpr
// Это делает возможным написание функций наподобие следующей:

// Возвращает отражение точки р
// относительно начала координат (С++14 )
constexpr Point reflection(const Point& p) noexcept {
    Point result;
    result.set_x(-p.x_value());
    result.set_y(-p.y_value());
    return result;
} 


int main() {
    std::array<int, pow11(2, 3)> arr{};
    static_assert(arr.size() == 8);

    std::array<int, pow14(2, 2)> arr2{};
    static_assert(arr2.size() == 4);



    constexpr Point p1(10,20);
    constexpr Point p2(30,40);
    constexpr auto mid = midpoint(p1, p2);
    constexpr auto ref = reflection(p1);
}
