#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

struct VectorInt {
    int* buffer;
    size_t sz;
    size_t cap;
};

int CreateVectorInt(struct VectorInt* vec, size_t cap){
    int* res = malloc(sizeof(int) * cap);
    if (res == NULL)
        return -1;
    vec->buffer = res;
    vec->sz = 0;
    vec->cap = cap;
    return 1;
}

int PushBackVectorInt(struct VectorInt* vec, int value) {
    if (vec->cap == vec->sz) {
        size_t new_cap = 2 * vec->cap;
        int* res = malloc(new_cap * sizeof(int));
        if (res==NULL) 
            return -1;
        memcpy(res, vec->buffer, vec->sz*sizeof(int));
        free(vec->buffer);
        vec->buffer = res;
        vec->cap = new_cap;
    }
    vec->buffer[vec->sz++] = value;
    return 0;
}

int GetValueVectorInt(struct VectorInt* vec, size_t index) {
    return vec->buffer[index];
}

void PrintElemsVectorInt(struct VectorInt* vec) {
    for(size_t i = 0;i<vec->sz;++i) {
        printf(" %d ", GetValueVectorInt(vec, i));
    }
    printf("\n");
}

void DeleteVectorInt(struct VectorInt* vec) {
    vec->sz = 0;
    vec->cap = 0;
    free(vec->buffer);
    vec->buffer = NULL;
}

int main() {
    struct VectorInt vec;
    if (CreateVectorInt(&vec, 5) == -1) {
        printf("CreateVectorInt error");
        return -1;
    }
    PushBackVectorInt(&vec, 20);
    PushBackVectorInt(&vec, 30);
    PushBackVectorInt(&vec, 40);
    PushBackVectorInt(&vec, 40);
    PushBackVectorInt(&vec, 40);

    PrintElemsVectorInt(&vec);

    PushBackVectorInt(&vec, 11);
    PushBackVectorInt(&vec, 22);
    PushBackVectorInt(&vec, 33);

    PrintElemsVectorInt(&vec);

    DeleteVectorInt(&vec);

    return 0;
}