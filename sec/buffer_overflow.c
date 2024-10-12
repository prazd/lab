#include <stdio.h>
#include <string.h>


//todo: finish
void vulnerable_function() {
    char buffer[100];
    gets(buffer);  // уязвимая функция, нет проверки длины строки
    printf("Input: %s\n", buffer);
}

int main() {
    vulnerable_function();
    return 0;
}
