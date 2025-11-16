void print(const char* str, unsigned long long len) {
    // write syscall
    asm volatile(          
        "mov $0x2000004, %%rax\n\t" // syscall num
        "mov $1, %%rdi\n\t" // fd(stdout)
        "mov %0, %%rsi\n\t" // str address
        "mov %1, %%rdx\n\t" // size
        "syscall\n\t"
        :
        : "r"(str), "r"(len)
        : "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
    );
}

int main(){
    print("Hello\n", 6);
}
