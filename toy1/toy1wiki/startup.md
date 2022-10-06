## 梳理好一个基本模块框架
想必各位曾经编写过基本的内核模块，这里就暂不赘述。
```C
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("w3w3w3");
MODULE_DESCRIPTION("LKM rootkit");
MODULE_VERSION("0.0.1");

static int __init mode_init(void) {
    printk(KERN_INFO "rootkit:init\n");
    return 0;
}

static void __exit mode_exit(void) {
    printk(KERN_INFO "rootkit:exit\n");
}

module_init(mode_init);
module_exit(mode_exit);
```

对应的makefile文件已经在文件中给出。由于涉及到系统路径，这里推荐在Ubuntu环境下使用，不推荐WSL。
## 寻找syscall
### How can we get Syscall table?
1. 'cat /proc/kallsyms | grep sys_call_table' 

(ps: you'd better do this in root mode, else the address can't be shown)

2. 'cat /proc/kallsyms' | grep kallsyms_lookup_name'

these are more relative to the kernel. In kernel source code we have seen that this function will lookup the address for a given name, and takes this as the return.

特别的，可以查阅kallsyms文件内的信息，得到内核内相关函数的符号表信息。linux内并没有提供sys_call_table的直接地址，实际装载时其值可能也有所不同，采用kallsyms_lookup_name查阅更为保险。

通过上面的查阅可以找到系统调用和内核调用函数表的基础地址。

### 注意事项
一些其他：建议在内核版本较为老旧的linux中进行实验，在较新版本中，进行make时会看到以下现象：
```
ERROR: modpost: "kallsyms_lookup_name" [/home/link/Desktop/toy1/ko.ko] undefined!
```
为避免上述问题，上述方法最好在较低版本内核使用（5.7.0以下），如果使用较为新的内核，可以参考这里的issue：

https://github.com/xcellerator/linux_kernel_hacking/issues/3