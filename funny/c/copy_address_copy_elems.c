#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

int tmult_ok(int a, int b) {
    if (a == 0 || b == 0) return 1;
    if (a > 0 && b > 0 && a > INT_MAX / b) return 0;  
    if (a < 0 && b < 0 && a < INT_MAX / b) return 0;
    if ((a > 0 && b < 0 && b < INT_MIN / a) || (a < 0 && b > 0 && a < INT_MIN / b)) return 0;
    return 1;
}


void* copy_elements(void* ele_src[], int ele_cnt, size_t ele_size)
{
    if (!tmult_ok(ele_cnt, ele_size))
        return NULL;
    printf("size: %zu\n", ele_cnt * ele_size);

    void* result = malloc(ele_cnt * ele_size);
    if (result == NULL)
        return NULL;

    void* next = result;
    printf("Loop addresses begin:\n");
    for (int i = 0; i < ele_cnt; i++)
    {
        printf("%p\n", ele_src[i]);
        memcpy(next, ele_src[i], ele_size);
        next += ele_size;
    }
    printf("Loop addresses end:\n");

    return result;
}

void* copy_addresses(void* ele_src[], int ele_cnt)
{
    int ele_size = sizeof(void*);
    int sz = ele_cnt * ele_size;
    if (!tmult_ok(ele_cnt, ele_size))
        return NULL;
    printf("size: %d\n", sz);

    void* result = malloc(sz);
    if (result == NULL)
        return NULL;
    
    memcpy(result, ele_src, sz);
    return result;
}

int main() {

    int a = 4;
    int b = 7;
    int c = 8;

    void* array[3] = {&a, &b, &c};

    int len = sizeof(array) / sizeof(array[0]);

    // sizeof int - 4 bytes (for copy_elements);
    int ele_size = sizeof(*((int*)array[0]));

    printf("a:%p\n", array[0]);
    printf("b:%p\n", array[1]);
    printf("c:%p\n", array[2]);

    printf("COPY ELEMENTS (int*): \n");
    int* array_2 = copy_elements(array, len, ele_size);
    printf("array_2[0]: %d\n", array_2[0]);
    printf("array_2[1]: %d\n", array_2[1]);
    printf("array_2[2]: %d\n\n\n", array_2[2]);
    free(array_2);
    printf("\n");


    printf("COPY ADDRESSES (int**): \n");
    int** array_3 = copy_addresses(array, len);
    printf("array_3[0]: %p\n", array_3[0]);
    printf("array_3[1]: %p\n", array_3[1]);
    printf("array_3[2]: %p\n", array_3[2]);

    printf("*(array_3[0]):  %d\n",  *(array_3[0]));
    printf("*(array_3[1]):  %d\n",  *(array_3[1]));
    printf("*(array_3[2]):  %d\n\n",  *(array_3[2]));

    // change a b c
    a = 42;
    b = 777;
    c = 1024;

    printf("AFTER CHANGING: \n\n");
    printf("*(array_3[0]):  %d\n",  *(array_3[0]));
    printf("*(array_3[1]):  %d\n",  *(array_3[1]));
    printf("*(array_3[2]):  %d\n",  *(array_3[2]));
    free(array_3);


    return 0;
}