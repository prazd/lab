#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <asm/io.h>

// Virtual address
//                48 47     39 38     30 29     21 20     12 11          0
// |++++++++++++++++|+++++++++|+++++++++|+++++++++|+++++++++|++++++++++++|
// | sign extension |  PML4   |   PDPT  |    PD   |    PT   |   offset   |
// |++++++++++++++++|+++++++++|+++++++++|+++++++++|+++++++++|++++++++++++|
//      16bit           9bit      9bit      9bit      9bit       12bit


typedef struct {
    void* base_va;
    phys_addr_t next_pa;
    int err;
} PtStep;

static PtStep step(phys_addr_t pa, unsigned long var_va, size_t offset){
    PtStep ps;
    void* va = memremap(pa, PAGE_SIZE, MEMREMAP_WB);
    if (!va){
        ps.base_va = NULL;
        ps.next_pa = 0;
        ps.err = -ENOMEM;
        return ps;
    }
    size_t* va_z = (size_t*)va;
    ps.base_va = va;
    ps.next_pa = ((phys_addr_t)(*(va_z + ((var_va >> offset) & 0x1ff)))) & 0xffffffffff000;
    ps.err = 0;
    return ps;
}

// PML4(L4) -> PDPT(L3) -> PD(L2) -> PT(L1)
static int __init m_init(void){
    printk(KERN_INFO "_______PAGE WALK_______");
    unsigned long var = 100;
    unsigned long var_va = (unsigned long)(&var);

    printk(KERN_INFO "VAR VA:%lx", (unsigned long)(&var));
    printk(KERN_INFO "VIRT_TO_PHYS:%llx", virt_to_phys(&var));

    phys_addr_t pml4_pa;
    asm volatile("movq %%cr3, %0":"=r"(pml4_pa));
    pml4_pa &= 0xffffffffff000;

    PtStep pml4_s = step(pml4_pa, var_va, 39);
    if (pml4_s.err) return pml4_s.err;
    
    PtStep pdpt_s = step(pml4_s.next_pa, var_va, 30);
    if (pdpt_s.err) {
        memunmap(pml4_s.base_va);
        return pdpt_s.err;
    }

    PtStep pd_s = step(pdpt_s.next_pa, var_va, 21);
    if (pd_s.err){
        memunmap(pml4_s.base_va);
        memunmap(pdpt_s.base_va);
        return pd_s.err;
    }

    PtStep pt_s = step(pd_s.next_pa, var_va, 12);
    if (pt_s.err){
        memunmap(pml4_s.base_va);
        memunmap(pdpt_s.base_va);
        memunmap(pd_s.base_va);
        return pt_s.err;
    }

    printk(KERN_INFO "PAGE WALK RES:%llx", (pt_s.next_pa + (var_va & 0xfff)));

    memunmap(pml4_s.base_va);
    memunmap(pdpt_s.base_va);
    memunmap(pd_s.base_va);
    memunmap(pt_s.base_va);
    printk(KERN_INFO "_______________________");
    return 0;
}
static void __exit m_exit(void){}
module_init(m_init);
module_exit(m_exit);
MODULE_LICENSE("GPL");