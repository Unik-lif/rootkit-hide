## About 'ls'
by using 'strace ls', we can see in terminal that:
```shell
openat(AT_FDCWD, ".", O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_DIRECTORY) = 3
fstat(3, {st_mode=S_IFDIR|0755, st_size=4096, ...}) = 0
getdents64(3, /* 15 entries */, 32768)  = 480
getdents64(3, /* 0 entries */, 32768)   = 0
close(3)                                = 0
fstat(1, {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0), ...}) = 0
write(1, "cgdb-0.8.0\t   PigchaClient_deb  "..., 67cgdb-0.8.0	   PigchaClient_deb  TEST	      valgrind-3.19.0.tar.bz2
) = 67
```
note that we see a function called getdents64, basically this syscall function will help us to use 'ls'.

https://linux.die.net/man/2/getdents64

We briefly conclude this function below:
```C
int getdents(unsigned int fd, struct linux_dirent *dirp,unsigned int count);
```
The system call getdents() reads several linux_dirent structures from the directory referred to by the open file descriptor fd into the buffer pointed to by dirp.

pay attention to the return value:
```
On success, the number of bytes read is returned. On end of directory, 0 is returned. On error, -1 is returned, and errno is set appropriately.
```

## About 'Process and files":
Firstly, let's look up this post.

https://tuxthink.blogspot.com/2012/05/module-to-print-open-files-of-process.html

In a process, we have $files\_struct$ to represent the files opened by this process. It also has $fdt$ to store all of the 'file descriptor's of the opened files.

```C
struct fdtable {
	unsigned int max_fds;
	struct file __rcu **fd;      /* current fd array */
	unsigned long *close_on_exec;
	unsigned long *open_fds;
	unsigned long *full_fds_bits;
	struct rcu_head rcu;
};
```
- what's the meaning of __rcu?
That's not important, but if interested, please refer to these post.
https://stackoverflow.com/questions/17128210/what-does-rcu-stands-for-in-linux
https://www.cnblogs.com/schips/p/linux_cru.html


remaining work.
```C
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
    d_inode = current->files->fdt->fd[fd]->f_dentry->d_inode;
#else
    d_inode = current->files->fdt->fd[fd]->f_path.dentry->d_inode;
#endif
```