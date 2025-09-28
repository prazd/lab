#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    unsigned long sz;
    // Struct Hack / Flexible array member / Trailing object pattern
    char buf[];
} S;

int main() {
    const char* str = "HelloWorld";
    unsigned long sz = strlen(str) + 1;
    void* ptr = malloc(sizeof(S) + sz);
    S* s_ptr = (S*)ptr;
    s_ptr->sz = sz;
    memcpy(s_ptr->buf, str, s_ptr->sz);;
    printf("%s", s_ptr->buf);
    free(ptr);
}
