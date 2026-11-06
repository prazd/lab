#include <linux/module.h>
#include <linux/kernel.h>

static int __init m_init(void){return 0;}

static void __exit m_exit(void){}

module_init(m_init);
module_exit(m_exit);
MODULE_LICENSE("GPL");