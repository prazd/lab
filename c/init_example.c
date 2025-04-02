// (VladimirovLectures 39)
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
    T a = {{.x = 10, .y = 20, .z = 30}, 5};
    T b = {{.xyz[0] = 10, .xyz[1] = 20, .xyz[2] = 30}, 5};
    T c[] = {[0].a = 1, [1].a = 10};
}