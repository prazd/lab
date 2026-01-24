#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#define VEC(T) typedef struct {T* begin; T* sz; T* cap;} Vector##T; \
    int CreateVector##T(Vector##T* vec, size_t cap){ \
        T* res = malloc(sizeof(T) * cap); \
        if (res == NULL) \
            return -1; \
        vec->begin = res; \
        vec->sz = res; \
        vec->cap = vec->begin + cap; \
        return 0; \
    }\
    int PushBackVector##T(Vector##T* vec, T value) { \
    if (vec->cap == vec->sz) { \
        size_t new_cap = 2 * (vec->cap - vec->begin); \
        size_t current_sz = (vec->sz - vec->begin); \
        T* res = malloc(new_cap * sizeof(T)); \
        if (res==NULL) \
            return -1; \
        memcpy(res, vec->begin, current_sz * sizeof(T)); \
        free(vec->begin); \
        vec->begin = res; \
        vec->sz = vec->begin + current_sz; \
        vec->cap = vec->begin + new_cap; \
    }\
        vec->begin[vec->sz - vec->begin] = value; \
        ++vec->sz; \
        return 0; \
    }\
    T GetValueVector##T(Vector##T* vec, size_t index) { \
        return vec->begin[index]; \
    }\
    void PrintElemsVector##T(Vector##T* vec, const char* str) { \
    size_t sz = vec->sz - vec->begin; \
        for(size_t i = 0; i < sz; ++i) { \
            printf(str, GetValueVector##T(vec, i)); \
        }\
        printf("\n");\
    } \
    void DeleteVector##T(Vector##T* vec) { \
        vec->sz = NULL; \
        vec->cap = NULL; \
        free(vec->begin); \
        vec->begin = NULL; \
    }


VEC(int)
VEC(float)

int vec_int_test() {
    printf("VecInt:\n");
    Vectorint vec;
    if (CreateVectorint(&vec, 5) == -1) {
        printf("CreateVectorInt error");
        return -1;
    }
    PushBackVectorint(&vec, 20);
    PushBackVectorint(&vec, 30);
    PushBackVectorint(&vec, 40);
    PushBackVectorint(&vec, 40);
    PushBackVectorint(&vec, 40);

    PrintElemsVectorint(&vec, " %d ");

    PushBackVectorint(&vec, 11);
    PushBackVectorint(&vec, 22);
    PushBackVectorint(&vec, 33);

    PrintElemsVectorint(&vec, " %d ");

    DeleteVectorint(&vec);
    return 0;
}

int vec_float_test() {
    printf("VecFloat:\n");
    Vectorfloat vec;
    if (CreateVectorfloat(&vec, 5) == -1) {
        printf("CreateVectorfloat error");
        return -1;
    }
    PushBackVectorfloat(&vec, 10.1f);
    PushBackVectorfloat(&vec, 11.2f);
    PushBackVectorfloat(&vec, 12.3f);
    PushBackVectorfloat(&vec, 13.4f);
    PushBackVectorfloat(&vec, 14.5f);

    PrintElemsVectorfloat(&vec, " %f ");

    PushBackVectorfloat(&vec, 11.1f);
    PushBackVectorfloat(&vec, 22.2f);
    PushBackVectorfloat(&vec, 33.3f);

    PrintElemsVectorfloat(&vec, " %f ");

    DeleteVectorfloat(&vec);
    return 0;
}

int main() {
    printf("%f\n", 10.2f);
    if (vec_int_test() == -1)
        return -1;    
    return vec_float_test();
}