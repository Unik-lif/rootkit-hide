Eidos(3364902617) 2022/10/8 20:08:04
接下来贴一下别的资料，https://9bie.org/index.php/archives/847/

Eidos(3364902617) 2022/10/8 20:08:05
https://docs-conquer-the-universe.readthedocs.io/zh_CN/latest/linux_rootkit/sys_call_table.html#sys-call-table

Eidos(3364902617) 2022/10/8 20:08:51
第二个文档年代较早，如果想早点对我们的进程隐藏和文件隐藏工作由头猪的话，请主要参阅9bie那个工作哈哈

Eidos(3364902617) 2022/10/9 11:12:28
https://notes.caijiqhx.top/security/linux_rootkit/

Eidos(3364902617) 2022/10/9 11:13:00
这份文档应该是信工所所里某个研三师兄做的资料，可以参考

Eidos(3364902617) 2022/10/9 11:13:44
考虑到成文的时间是2020fa，应该对应的内核版本是5.4，可以尝试跟一下

Eidos(3364902617) 2022/10/9 17:23:13
我太dinner了，toy2的文档看不下去

Eidos(3364902617) 2022/10/9 17:23:53
jm33的那个大家暂时别看吧，讲得不是很详细。

Eidos(3364902617) 2022/10/9 17:24:39
尤其是linux文件系统那一块儿在做进程和文件隐藏的时候没有做任何解释

Eidos(3364902617) 2022/10/9 17:28:00
先把推送放上去了，大致只要知道在hook之后利用current进程寻觅inode去寻找文件就行（至于隐藏，toy2之中有点语焉不详）

Eidos(3364902617) 2022/10/9 17:42:42
（toy2作废，不写代码，当做补充资料集合即可）

https://arttnba3.cn/2021/07/07/CODE-0X01-ROOTKIT/#0x00-%E6%A6%82%E8%BF%B0

文件隐藏：process：task_struct-->file_struct-->fdtable
遍历一下fdtable里头内容，fd s.


fopen()
fclose()
FILE *fp = fopen("filepath.txt", "r+");
file descriptor: 文件描述符