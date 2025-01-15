#include <cassert>

int sumeven(int* x, int n) {
    int sum = 0;
    for (int i = 0; i < n; ++i){
        if (x[i]%2 == 0) {
            sum+=x[i];
        }
    }
    return sum;
}

// int sumeven(int* x, int n) {
//
//  BB1: sum = 0; i = 0;
//     goto LOOP:
//
//  LOOP: 
//      if (i < n) goto BB2; else goto BB5;
//
//  BB2: tmp = x + i; elt = *tmp;
//      odd = elt % 2;
//      if (odd != 0) goto BB4; else goto BB3;
//
//  BB3: tmp = x + i; elt = *tmp;
//     sum = sum + elt;
//     goto BB4;
//
//  BB4: i = i + 1;
//     goto LOOP;
// 
//  BB5:
//    return sum;
//}

int main() {
    int x[8]{1,10,23,5,4232,12,123,123};
    assert(sumeven(x, 8) == (10 + 4232 + 12));
}