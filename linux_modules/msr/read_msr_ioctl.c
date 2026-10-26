#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <asm/io.h>

static unsigned long read_msr(unsigned long rcx){
    unsigned long rax = 0;
    unsigned long rdx = 0;
    asm volatile("rdmsr":"=a"(rax), "=d"(rdx) : "c"(rcx));
    return (rdx << 32) | rax;
}    

#define IOCTL_READ_MSR 'p'
#define IOCTL_READ_MSR_HANDLE \
    _IOWR(IOCTL_READ_MSR, 1, unsigned long)


static long handle(struct file* file, unsigned cmd, unsigned long arg){
    switch(cmd){
        case IOCTL_READ_MSR_HANDLE: {
            unsigned long rax;
            if (copy_from_user(&rax, (void*)arg, sizeof(rax))){
                return -EFAULT;
            }
            unsigned long res = read_msr(rax);
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
    .name = "ioctl_read_msr",
    .fops = &module_fops,
};

static int __init m_init(void){return misc_register(&device);}
static void __exit m_exit(void){misc_deregister(&device);}

module_init(m_init);
module_exit(m_exit);
MODULE_LICENSE("GPL");