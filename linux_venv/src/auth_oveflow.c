#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int auth(const char* str) {
    char buffer[10];
    strcpy(buffer, str);
    if (strcmp(buffer, "prazd") == 0) {
        return 1;
   }
   return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2){
        printf("Error");
        return -1;
    }

    if (auth(argv[1])) {
        printf("Logged int");
    } else {
        printf("Error");
    }

    return 0;
}
