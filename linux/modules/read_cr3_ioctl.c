#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

#define READ_CR3_IOC_MAGIC 'p'
#define READ_CR3 \
    _IOR(READ_CR3_IOC_MAGIC, 1, unsigned long)

static long handle(struct file* file, unsigned cmd, unsigned long arg){
    switch(cmd){
        case READ_CR3: {
            unsigned long cr3;
            asm volatile("movq %%cr3, %0":"=r"(cr3));
            if (copy_to_user((void __user*)arg, &cr3, sizeof(cr3))){
                return -EFAULT;
            }
            return 0;
        }
    }
    return -EINVAL;
}

static const struct file_operations read_cr3_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = handle,
};

static struct miscdevice device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "read_cr3",
    .fops = &read_cr3_fops,
};

static int __init m_init(void){return misc_register(&device);}

static void __exit m_exit(void){misc_deregister(&device);}

module_init(m_init);
module_exit(m_exit);
MODULE_LICENSE("GPL");