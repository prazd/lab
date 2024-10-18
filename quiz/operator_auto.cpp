#include <iostream>

struct Foo {
  operator auto() {
    std::cout << "A";
    return 1;
  }
};

int main() {
  int a = Foo();
  std::cout << a;
}

// A normal conversion function can have a deduced return type (only conversion function templates are not allowed to have one due to §[class.conv.fct]¶9):

// A conversion function template shall not have a deduced return type (§[dcl.spec.auto]).

// And even if conversion functions don't have a return type specified in the same way as normal functions, they do have a return type: §[class.conv.fct]¶3:

// The type of the conversion function is “noexceptopt function taking no parameter cv-qualifier-seqopt ref-qualifieropt returning conversion-type-id”.

// Where conversion-type-id is the T in operator T() {...}. So we're allowed to deduce this type just like we deduce return types from normal functions.