#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <asm/io.h>

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

#define IOCTL_READ_DR 'p'
#define IOCTL_READ_DR_HANDLE \
    _IOWR(IOCTL_READ_DR, 1, DRState)


static long handle(struct file* file, unsigned cmd, unsigned long arg){
    switch(cmd){
        case IOCTL_READ_DR_HANDLE: {
            DRState res = read_dr();
            if (copy_to_user((void __user*)arg, &res, sizeof(res))){
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
    .name = "ioctl_read_dr",
    .fops = &module_fops,
};

static int __init m_init(void){return misc_register(&device);}
static void __exit m_exit(void){misc_deregister(&device);}

module_init(m_init);
module_exit(m_exit);
MODULE_LICENSE("GPL");