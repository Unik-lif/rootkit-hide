#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("w3w3w3");
MODULE_DESCRIPTION("LKM rootkit");
MODULE_VERSION("0.0.1");

static int __init mode_init(void) {
    printk(KERN_INFO "rootkit:init\n");
    return 0;
}

static void __exit mode_exit(void) {
    printk(KERN_INFO "rootkit:exit\n");
}

module_init(mode_init);
module_exit(mode_exit);