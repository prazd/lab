#include <linux/module.h>
#include <linux/kernel.h>

enum IA32_EFER_BIT_VAL {
    SYSCALL_SYSRET_ENABLE=0x0,
    LONG_MODE_ENABLE=0x8,
    LONG_MODE_ACTIVE=0xa,
    NX_ENABLE=0xb,
    AMD_SVM_ENABLE=0xc,
    FFXSR=0xe,
    AMD_TRANSLATION_CACHE_EXTENSION=0xf,
};

#define PRINT_IA32(flag,res) printk(#flag":%lu",(res >> flag) & 1);
static int __init read_msr(void){
    unsigned long rax = 0; // low 
    unsigned long rdx = 0; // high
    asm volatile("rdmsr":"=a"(rax),"=d"(rdx) : "c"(0xC0000080));
    unsigned long res = ((rdx << 32) | rax);

    printk(KERN_INFO "\n_____RDMSR IA32 EFER_____\n");
    PRINT_IA32(SYSCALL_SYSRET_ENABLE,res)
    PRINT_IA32(LONG_MODE_ENABLE,res)
    PRINT_IA32(LONG_MODE_ACTIVE,res)
    PRINT_IA32(NX_ENABLE,res)
    PRINT_IA32(AMD_SVM_ENABLE,res)
    PRINT_IA32(FFXSR,res)
    PRINT_IA32(AMD_TRANSLATION_CACHE_EXTENSION,res)
    printk(KERN_INFO "_________________________\n");
    return 0;
}

static void __exit print_unload(void){
    pr_info("read msr unload\n");
}

module_init(read_msr);
module_exit(print_unload);
MODULE_LICENSE("GPL");
