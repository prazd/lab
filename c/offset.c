#include <stdio.h>

#define OFFSET(s, f) (unsigned long)(&(((s*)0)->f))

typedef struct {
    int x;
    short y;
    int z;
} S;

int main(){
    printf("S.x offset:%lu\n", OFFSET(S, x));
    printf("S.y offset:%lu\n", OFFSET(S,  y));
    printf("S.z offset:%lu", OFFSET(S, z));
}
