#include "stdio.h"

void implementation1(){printf("imp1\n");}
void implementation2(){printf("imp2\n");}
 
void (*resolve_func())(){
    if (__builtin_cpu_supports("avx"))
        return implementation1;
    return implementation2;    
}

void func() __attribute__((ifunc ("resolve_func")));

int main(){
    func();
}
