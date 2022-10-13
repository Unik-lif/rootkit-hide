## 文件隐藏
### 文件夹与文件，以及文件操作函数
在前述的toy2中，我们大致提到了函数调用流程的一个技法：
```shell
strace <command>
```
在这边，我们可以着重看一下getdents64函数，当然，这是第二次强调了。
```C
int getdents(unsigned int fd, struct linux_dirent *dirp,unsigned int count);
```
我们对几个数据结构做简要解析，首先是fd结构体下的信息：
```C
struct fd {
	struct file *file;
	unsigned int flags;
};
```
因此，我们可以通过fd索引到我们想要的文件指针。我们继续查看file结构体的信息，file结构体下的信息如下所示：
```C
struct file {
    ......
	struct path		f_path;
	struct inode		*f_inode;	/* cached value */
	const struct file_operations	*f_op;
    ......
};
```
我们注意到存在一个名为file_operations的结构体，这个结构体定义如下，将在很多文件的处理函数中出现。
```C
struct file_operations {
    .....
	int (*iterate) (struct file *, struct dir_context *);
	int (*iterate_shared) (struct file *, struct dir_context *);
    .....
}
```
特别的，我们应记得其存在两个函数，其一为iterate，其二为iterate_shared，其功能近似，均为遍历文件夹下的文件所执行的操作。
### 函数调用流程：
```C
SYSCALL_DEFINE3(getdents, unsigned int, fd,
		struct linux_dirent __user *, dirent, unsigned int, count)
{
	struct fd f;
	struct getdents_callback buf = {
		.ctx.actor = filldir,
		.count = count,
		.current_dir = dirent
	};
	int error;

	if (!access_ok(dirent, count))
		return -EFAULT;

	f = fdget_pos(fd);
	if (!f.file)
		return -EBADF;

	error = iterate_dir(f.file, &buf.ctx);
	if (error >= 0)
		error = buf.error;
	if (buf.prev_reclen) {
		struct linux_dirent __user * lastdirent;
		lastdirent = (void __user *)buf.current_dir - buf.prev_reclen;

		if (put_user(buf.ctx.pos, &lastdirent->d_off))
			error = -EFAULT;
		else
			error = count - buf.count;
	}
	fdput_pos(f);
	return error;
}
```
注意到，上面给ctx.actor载荷赋上了一个值叫做filldir，表示使用这个函数。

在ctx中涉及的函数和数据结构如下所示：
```C
struct dir_context {
	filldir_t actor;
	loff_t pos;
};
```

```C
/*
 * This is the "filldir" function type, used by readdir() to let
 * the kernel specify what kind of dirent layout it wants to have.
 * This allows the kernel to read directories into kernel space or
 * to have different dirent layouts depending on the binary type.
 */
typedef int (*filldir_t)(struct dir_context *, const char *, int, loff_t, u64, unsigned);
```

我们继续向下搜索：可以看到iterate_dir被调用，该函数中有这么几行：
```C
int iterate_dir(struct file *file, struct dir_context *ctx)
{
		....
		if (shared)
			res = file->f_op->iterate_shared(file, ctx);
		else
			res = file->f_op->iterate(file, ctx);
		....
}
```
这两个函数是根据文件系统类型赋值的，那么很自然，我们要搞懂对应文件系统的函数喽，再去寻找他们。

特别的，利用命令查看文件系统的类型是什么：
```shell
root@ubuntu:/home/link/Desktop/libre/backdoor# df -Th
Filesystem     Type      Size  Used Avail Use% Mounted on
udev           devtmpfs  1.9G     0  1.9G   0% /dev
tmpfs          tmpfs     391M  2.0M  389M   1% /run
/dev/sda5      ext4       49G   13G   34G  28% /
......
```
答案呼之欲出，应该是ext4类型，那么，先前追溯的iterate调用的应该就是ext4系列的函数了，实际上就是下面这个函数。

我们接下来参考一下这篇文章（妈妈救命，这个太难了）：

https://zhuanlan.zhihu.com/p/129078046

接下来我们再看看这边的调用链：
```C
// when using iterate_shared or iterate, under ext4 filesystem, we call ext4_readdir.
static int ext4_readdir(struct file * file, struct dir_context *ctx);

static int ext4_dx_readdir(struct file *, struct dir_context *);

static int call_filldir(struct file *file, struct dir_context *ctx, struct fname *fname);

dir_emit(ctx, fname->name, fname->name_len, fname->inode, get_dtype(sb, fname->file_type)))

static inline bool dir_emit(struct dir_context *ctx, const char *name, int namelen, u64 ino, unsigned type) {
	return ctx->actor(ctx, name, namelen, ctx->pos, ino, type) == 0;
}
```
行了，也就是说，终于我们看到ctx使用了actor载荷，也就是运行了filldir函数。

终于到这一步了，那么我们可以看看filldir函数做了什么事情：

具体来说，filldir将记录填入返回的缓冲区中。如果我们不把记录填入缓冲区，那么其余函数和应用程序就收不到记录。

参考资料:

https://stackoverflow.com/questions/49128739/how-to-access-the-proc-file-systems-iiterate-function-pointer

