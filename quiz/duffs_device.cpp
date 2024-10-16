#include <iostream>

int main() {
    int n = 3;
    int i = 0;

    switch (n % 2) {
    case 0:
    do {
    ++i;
    case 1: ++i;
    } while (--n > 0);
    }

    std::cout << i;
}

// Though this piece of code might look strange at first sight, it is still valid C++ code and is known as Duff's device.
// According to the standard §[stmt.select]¶1, a switch statement can be followed (likewise, e.g., an if statement) by any valid statement including a compound (block) statement.
// The only difference from a general compound statement is that case and default labels can appear inside it. 
// Jumping into the nested do-while block is legal (§[stmt.dcl]¶3), and its execution is not altered by the case labels (§[stmt.switch]¶6).