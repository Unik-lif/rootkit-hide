#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kallsyms.h> // this will give us access to kallsys_lookup_name function.

MODULE_LICENSE("GPL");
MODULE_AUTHOR("w3w3w3");
MODULE_DESCRIPTION("LKM rootkit");
MODULE_VERSION("0.0.1");

unsigned long * __sys_call_table = NULL;

static int __init mode_init(void) {
    printk(KERN_INFO "rootkit:init\n");

    __sys_call_table = (unsinged long *) kallsyms_lookup_name("sys_call_table");
    return 0;
}

static void __exit mode_exit(void) {
    printk(KERN_INFO "rootkit:exit\n");
}

module_init(mod_init);
module_exit(mod_exit);