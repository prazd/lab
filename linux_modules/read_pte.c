#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/mm.h>


// PML4 (L4) -> PDPT(L3) -> PD(L2) -> PT(L1)
static int __init m_init(void){
    unsigned long pml4_pa; // TODO: check phys_addr_t 
    asm volatile("movq %%cr3, %0":"=r"(pml4_pa));
    pml4_pa &= 0xffffffffff000; // pml4_pa &= PAGE_MASK

    unsigned char* pml4_va = (unsigned char*)memremap(pml4_pa, PAGE_SIZE, MEMREMAP_WB);
    if (!pml4_va){return -ENOMEM;}
    size_t* pml4_va_z = (size_t*)pml4_va;
    for (size_t i = 0, e = PAGE_SIZE/sizeof(size_t); i != e; ++i)
        printk(KERN_INFO "PML4_E[%zx]:%zx", i, *(pml4_va_z + i));
    // TODO:finish
    memunmap(pml4_va);
    return 0;
}
static void __exit m_exit(void){}
module_init(m_init);
module_exit(m_exit);
MODULE_LICENSE("GPL");
