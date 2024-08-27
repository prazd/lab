#include <iostream>

// завершающий возвраща­ емый тип (trailing return type), т.е. что возвращаемый тип функции 
// будет объявлен по­ сле списка параметров (после "->"). 

// Завершающий возвращаемый тип обладает тем пре­ имуществом, что в 
// спецификации возвращаемого типа могут использоваться параметры функции.
template<typename Container, typename Index>
auto auth_and_access(Container& c, Index i) -> decltype(c[i]) {
    // ... auth logic
    return c[i];
}

int main() {
    std::vector<int> a = {100,200,300};
    auto& el = auth_and_access(a, 1);
}
