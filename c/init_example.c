#include <assert.h>
typedef struct {
    union {
        struct {
            int x, y, z;
        };
        int xyz[3];
    };
    int a;
} T;

int main() {
    // Vladimirov lectures(39)
    T a = {{.x = 10, .y = 20, .z = 30}, 5};
    T b = {{.xyz[0] = 10, .xyz[1] = 20, .xyz[2] = 30}, 5};
    T c[] = {[0].a = 1, [1].a = 10};
   
    //
    int arr[4] = {[0 ... 2] = 10, [3] = 5};
    assert(arr[0] == 10);
    assert(arr[1] == 10);
    assert(arr[2] == 10);
    assert(arr[3] == 5);
   
}
