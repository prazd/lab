#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <asm/io.h>

typedef struct {
    phys_addr_t next_pa;
    int err;
} PtStep;

static PtStep step(phys_addr_t pa, unsigned long var_va, size_t offset){
    PtStep ps;
    void* va = memremap(pa, PAGE_SIZE, MEMREMAP_WB);
    if (!va){
        ps.next_pa = 0;
        ps.err = -ENOMEM;
        return ps;
    } 
    size_t* va_z = (size_t*)va;
    ps.next_pa = ((phys_addr_t)(*(va_z + ((var_va >> offset) & 0x1ff))));
    ps.err = 0;
    memunmap(va);
    return ps;
}

static long read_huge_page(unsigned long res_pa, unsigned long var_va, unsigned long mask) {
    unsigned long* res_va = (unsigned long*)memremap(res_pa & mask, (((~mask) & 0xfffffffffffff) + 1), MEMREMAP_WB);
    long res = *((long*)(((unsigned long)(res_va)) | (var_va & ((~mask) & 0x3fffffff)))); // max huge page mask - 1Gib
    memunmap(res_va);
    return res;
}

// TODO: handle huge pages
// PML4(L4) -> PDPT(L3) -> PD(L2) -> PT(L1)
static long page_walk(unsigned long var_va){
    phys_addr_t pml4_pa;
    asm volatile("movq %%cr3, %0":"=r"(pml4_pa));
  
    // maybe CR3_ADDR_MASK
    PtStep pml4_s = step(pml4_pa & PAGE_MASK, var_va, 39);
    if (pml4_s.err) return pml4_s.err;
    
    PtStep pdpt_s = step(pml4_s.next_pa & PAGE_MASK, var_va, 30);
    if (pdpt_s.err) return pdpt_s.err;
    if (pdpt_s.next_pa & 0x80) return read_huge_page(pdpt_s.next_pa, var_va, 0xfffffc0000000); // 1Gib 
    
    PtStep pd_s = step(pdpt_s.next_pa & PAGE_MASK, var_va, 21);
    if (pd_s.err) return pd_s.err;
    if (pd_s.next_pa & 0x80) return read_huge_page(pd_s.next_pa, var_va, 0xfffffffe00000); // 2Mib

    PtStep pt_s = step(pd_s.next_pa & PAGE_MASK, var_va, 12);
    if (pt_s.err) return pt_s.err;
    
    phys_addr_t res_pa = ((pt_s.next_pa & PAGE_MASK));
    unsigned long* res_va = (unsigned long*)memremap(res_pa & 0xffffffffff000, PAGE_SIZE, MEMREMAP_WB); // 4Kib
    unsigned long res = *((unsigned long*)(((unsigned long)(res_va)) | (var_va & 0xfff)));
    memunmap(res_va);
    return res;
}    

typedef struct {
    unsigned long va;
    long res;
} page_walk_req;

#define IOCTL_PAGE_WALK 'p'
#define IOCTL_PAGE_WALK_HANDLE \
    _IOWR(IOCTL_PAGE_WALK, 1, page_walk_req)

static long handle(struct file* file, unsigned cmd, unsigned long arg){
    switch(cmd){
        case IOCTL_PAGE_WALK_HANDLE: {
            page_walk_req req;
            if (copy_from_user(&req, (void*)arg, sizeof(req))){
                return -EFAULT;
            }
            
            req.res = page_walk(req.va);
            if (copy_to_user((void __user*)arg, &req, sizeof(req))){
                return -EFAULT;
            }
            printk(KERN_INFO "___________________________");
            return 0;
        }
    }
    return -EINVAL;
}

static const struct file_operations module_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = handle,
};

static struct miscdevice device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "ioctl_page_walk",
    .fops = &module_fops,
};

static int __init m_init(void){return misc_register(&device);}

static void __exit m_exit(void){misc_deregister(&device);}

module_init(m_init);
module_exit(m_exit);
MODULE_LICENSE("GPL");