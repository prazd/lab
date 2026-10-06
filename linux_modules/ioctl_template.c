#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#define MODULE_MAGIC 'p'
#define MODULE_HANDLE \
    _IOR(READ_CR3_IOC_MAGIC, 1, unsigned long) // _IOW/

static long handle(struct file* file, unsigned cmd, unsigned long arg){
    switch(cmd){
        case MODULE_HANDLE: {return 0;}
    }
    return -EINVAL;
}

static const struct file_operations module_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = handle,
};

static struct miscdevice device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "...",
    .fops = &module_fops,
};

static int __init m_init(void){return misc_register(&device);}

static void __exit m_exit(void){misc_deregister(&device);}

module_init(m_init);
module_exit(m_exit);
MODULE_LICENSE("GPL");