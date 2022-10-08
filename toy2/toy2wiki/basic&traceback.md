## what kinda syscall might be used?
Basically we can use strace to see the function call. Taken cat as an example.
```shell
$ strace cat /tmp/test
........
openat(AT_FDCWD, "/tmp/test", O_RDONLY) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=5, ...}) = 0
fadvise64(3, 0, 0, POSIX_FADV_SEQUENTIAL) = 0
mmap(NULL, 139264, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7fe15c9e5000
read(3, "test\n", 131072)               = 5
write(1, "test\n", 5test
)                   = 5
```
if we hook openat, we can tell cat that this file doesn't exist, or give it Permission denied.

## another way to reference and finding sys_call_table.

```C
unsigned long* get_syscall_table_bf(void)
{
    unsigned long* syscall_table;
    unsigned long int i;

    for (i = (unsigned long int)sys_close; i < ULONG_MAX;
         i += sizeof(void*)) {
        syscall_table = (unsigned long*)i;

        if (syscall_table[__NR_close] == (unsigned long)sys_close)
            return syscall_table;
    }
    return NULL;
}
```
By referencing the elixir, we find that in <linux/syscall.h> we can find the address of 'sys_close'. This function happens be of lower address than 'sys_call_table'.

Therefore, we can use this way to fetch the address of the sys_call_table.(a simple judge is enough)

Different from the LKM version of LINUX_VERSION_CODE greater than 4.4.0, this method won't use 'kallsyms_lookup_name'.

I should lanuch a simple test on this as well.
the result will be given here:)

### Check lab:
I check this message in my own virtual machine:
```shell
link@ubuntu:~/Desktop$ sudo cat /proc/kallsyms | 
grep sys_close
ffffffffb40c4e20 T __ia32_sys_close
ffffffffb40c55c0 T __x64_sys_close
ffffffffb58ecb40 t _eil_addr___ia32_sys_close
ffffffffb58ecb50 t _eil_addr___x64_sys_close
link@ubuntu:~/Desktop$ sudo cat /proc/kallsyms | grep sys_call_table
ffffffffb4e00280 R x32_sys_call_table
ffffffffb4e013a0 R sys_call_table
ffffffffb4e023e0 R ia32_sys_call_table
```
We can conclude that sys_close 's address is smaller than sys_call_table.

However, due to some modern linux kernel, and my test lab, even using 'linux/syscalls.h' as the header, we still can't find 'sys_call', and meantime, we don't know the address of the 'ksys_call'.

**So, this idea should be given up. It's too old!**
