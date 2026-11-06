#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <asm/io.h>


static unsigned long rdmsr_f(unsigned long rcx){
    unsigned long rax = 0;
    unsigned long rdx = 0;
    asm volatile("rdmsr":"=a"(rax), "=d"(rdx) : "c"(rcx));
    return (rdx << 32) | rax;
}

static unsigned long rdpmc_f(unsigned long rcx){
    unsigned long rax = 0;
    unsigned long rdx = 0;
    asm volatile("rdpmc":"=a"(rax), "=d"(rdx) : "c"(rcx));
    return (rdx << 32) | rax;
}  

typedef struct {
    uintptr_t regs[8];
    int err;
} DRState;

static DRState read_dr(void){
    DRState drs;
    asm volatile("mov %%dr0, %0":"=r"(drs.regs[0]));
    asm volatile("mov %%dr1, %0":"=r"(drs.regs[1]));
    asm volatile("mov %%dr2, %0":"=r"(drs.regs[2]));
    asm volatile("mov %%dr3, %0":"=r"(drs.regs[3]));
    asm volatile("mov %%dr4, %0":"=r"(drs.regs[4]));
    asm volatile("mov %%dr5, %0":"=r"(drs.regs[5]));
    asm volatile("mov %%dr6, %0":"=r"(drs.regs[6]));
    asm volatile("mov %%dr7, %0":"=r"(drs.regs[7]));
    return drs;
} 


#define IOCTL_CALL 'p'

#define IOCTL_RDMSR_HANDLE \
    _IOWR(IOCTL_CALL, 0, unsigned long) \

#define IOCTL_RDPMC_HANDLE \
    _IOWR(IOCTL_CALL, 1, unsigned long) \

#define IOCTL_READ_DR_HANDLE \
    _IOWR(IOCTL_CALL, 2, DRState)  \

static long handle(struct file* file, unsigned cmd, unsigned long arg){
    switch(cmd){
        case IOCTL_RDMSR_HANDLE: {
            unsigned long rcx = 0;
            if (copy_from_user(&rcx, (void*)arg, sizeof(rcx)))
                return -EFAULT;

            unsigned long res = rdmsr_f(rcx);
            return copy_to_user((void __user*)arg, &res, sizeof(res)) ? -EFAULT : 0;
        }
        case IOCTL_RDPMC_HANDLE: {
            unsigned long rcx = 0;
            if (copy_from_user(&rcx, (void*)arg, sizeof(rcx)))
                return -EFAULT;

            unsigned long res = rdpmc_f(rcx);
            return copy_to_user((void __user*)arg, &res, sizeof(res)) ? -EFAULT : 0;
        }
        case IOCTL_READ_DR_HANDLE: {
            DRState res = read_dr();
            return copy_to_user((void __user*)arg, &res, sizeof(res)) ? -EFAULT : 0;
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
    .name = "ioctl_call",
    .fops = &module_fops,
};

static int __init m_init(void){return misc_register(&device);}
static void __exit m_exit(void){misc_deregister(&device);}

module_init(m_init);
module_exit(m_exit);
MODULE_LICENSE("GPL");