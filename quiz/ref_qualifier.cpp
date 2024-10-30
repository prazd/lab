#include <type_traits>
#include <iostream>

// §[dcl.fct]¶12 says:
// The return type, the parameter-type-list, the ref-qualifier, the cv-qualifier-seq, and the exception specification, but not the default arguments (§[dcl.fct.default]) or the trailing requires-clause (§[dcl.decl]), are part of the function type.
// This means that the ref-qualifier (in this case const&&) is part of the type and thus the type of ptr is int(X::*)() const&&.

struct X {
    int f() const&& {
        return 0;
    }
};

template<typename T>
void Delete(T) = delete;

int main() {
    auto ptr = &X::f;
    std::cout << std::is_same_v<decltype(ptr), int()>
              << std::is_same_v<decltype(ptr), int(X::*)()>
              << std::is_same_v<decltype(ptr), int(X::*)()const&&>;
}