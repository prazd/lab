#include <stdio.h>

void __attribute__((constructor)) foo() {
    printf("Before main\n");
}
int main() {
    printf("Main");
    return 0;
}
