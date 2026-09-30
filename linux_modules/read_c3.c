#include <linux/module.h>
#include <linux/kernel.h>

static int __init read_c3(void){
    unsigned long res;
    asm("movq %%cr3, %0":"=r"(res));
    printk(KERN_INFO "C3:%p\n",(void*)res);
    return 0;
}

static void __exit print_unload(void){
    pr_info("read c3 unload\n");
}

module_init(read_c3);
module_exit(print_unload);
MODULE_LICENSE("GPL");
