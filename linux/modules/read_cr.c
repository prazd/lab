#include <linux/module.h>
#include <linux/kernel.h>

enum CONTROL_REGISTER_BIT_POS {
    CR0_PROTECTION_ENABLE=0x0,
    CR0_PAGING_ENABLE=0x1f,
    CR0_WRITE_PROTECT=0x10,
    CR0_TASK_SWITCHED=0x3,
    
    CR4_PHYSICAL_ADDRESS_EXTENSION=0x5,
    CR4_PAGE_SIZE_EXTENSIONS=0x4,
    CR4_SMEP=0x14,
    CR4_SMAP=0x15,
    CR4_USER_MODE_INSTRUCTION_PREVENTION=0xb,
    CR4_PCIDE=0x11,
};

#define PRINT_CR_BIT_VAL(res,pos) printk(KERN_INFO #pos":%lu\n",(res >> pos) & 1);

static int __init read_c3(void){
    printk(KERN_INFO "__________CONTROL_REGISTERS__________");
    unsigned long res;

    asm("movq %%cr0, %0":"=r"(res));
    PRINT_CR_BIT_VAL(res, CR0_PROTECTION_ENABLE)
    PRINT_CR_BIT_VAL(res, CR0_PAGING_ENABLE)
    PRINT_CR_BIT_VAL(res, CR0_WRITE_PROTECT)
    PRINT_CR_BIT_VAL(res, CR0_TASK_SWITCHED)

    asm("movq %%cr2, %0":"=r"(res));
    printk(KERN_INFO "C2_PAGE_FAULT_LINEAR_ADDRESS:%p\n",(void*)res);

    asm("movq %%cr4, %0":"=r"(res));
    PRINT_CR_BIT_VAL(res, CR4_PHYSICAL_ADDRESS_EXTENSION)
    PRINT_CR_BIT_VAL(res, CR4_PAGE_SIZE_EXTENSIONS)
    PRINT_CR_BIT_VAL(res, CR4_SMEP)
    PRINT_CR_BIT_VAL(res, CR4_SMAP)
    PRINT_CR_BIT_VAL(res, CR4_USER_MODE_INSTRUCTION_PREVENTION)
    PRINT_CR_BIT_VAL(res, CR4_PCIDE)
    
    bool is_cr4_pcide = (res >> CR4_PCIDE) & 1;

    asm("movq %%cr3, %0":"=r"(res));
    if (is_cr4_pcide) printk(KERN_INFO "C3_PCID:%lu\n",res & 0xfff);
    printk(KERN_INFO "C3_PML4_PHYSYCAL_ADDRESS:%p\n",(void*)((res >> 12) & 0xffffffffff));

    asm("movq %%cr8, %0":"=r"(res));
    printk(KERN_INFO "CR8_TASK_PRIORITY_MANAGER:%lu\n", res);
    
    
    printk(KERN_INFO "_____________________________________");
    return 0;
}

static void __exit print_unload(void){
    pr_info("read c3 unload\n");
}

module_init(read_c3);
module_exit(print_unload);
MODULE_LICENSE("GPL");
