#include "stdafx.h"

void foo() {
    std::vector<int> v{};
    std::iota(v.begin(), v.end(), 0);
}
