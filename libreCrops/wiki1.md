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

