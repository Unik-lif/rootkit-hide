## 端口隐藏

### 到底是哪个函数？
鄙人对于网络方面并不太熟悉，先进行一些知识储备记录。

采用下面的措施：
```shell
cat /proc/net/tcp
```
可以帮助我们得到一些TCP_SEQ_STATE_LISTENING或者TCP_SEQ_STATE_ESTABLISHED情况。

平常如果为了得到该文件下的一些信息，我们会采用命令：
```shell
netstat
```
运行上述命令后得到的信息可以参阅文档，特别的我们可以注意到一些端口和ip信息。

https://guanjunjian.github.io/2017/11/09/study-8-proc-net-tcp-analysis/

该资料同时也详细介绍了我们的核心函数tcp4_seq_show函数。

```C
static int tcp4_seq_show(struct seq_file *seq, void *v) {
	struct tcp_iter_state *st;
	struct sock *sk = v;

	seq_setwidth(seq, TMPSZ - 1);
	if (v == SEQ_START_TOKEN) {
		seq_puts(seq, "  sl  local_address rem_address   st tx_queue "
			   "rx_queue tr tm->when retrnsmt   uid  timeout "
			   "inode");
		goto out;
	}
	st = seq->private;

	if (sk->sk_state == TCP_TIME_WAIT)
		get_timewait4_sock(v, seq, st->num);
	else if (sk->sk_state == TCP_NEW_SYN_RECV)
		get_openreq4(v, seq, st->num);
	else
		get_tcp4_sock(v, seq, st->num);
out:
	seq_pad(seq, '\n');
	return 0;
}

struct seq_file {
	char *buf;
	size_t size;
	size_t from;
	size_t count;
	size_t pad_until;
	loff_t index;
	loff_t read_pos;
	u64 version;
	struct mutex lock;
	const struct seq_operations *op;
	int poll_event;
	const struct file *file;
	void *private;
};
```
特别的，内核利用seq_file结构体作为接口，而该结构体中存在较为重要的seq_operations这一载荷。

我们注意到函数tcp4_seq_show其本质是一个很handy的打印函数，其会根据读入的seq_file信息进行打印，并再根据private载荷内的sk_state状态来对seq_file执行不同的操作。

我们查看了在后续的if else块中指向的三个函数，发现了一个共同点，其均采用seq_printf函数将我们的seq信息逐条打印出来。

此外，即便在先前经过了goto out指令跳出了这个部分代码块，我们发现在seq_puts函数之中也会进行打印。

合理猜测，这个名为tcp4_seq_show的函数为tcp/ipv4下我们需要挂钩的函数。当然，tcp/ipv4和其余类型则是同理，我们在这里仅以tcp/ipv4为例。

## 调用流程
### 文件夹是怎么建立的？
本部分主要参考了下面的资料：

https://dandelioncloud.cn/article/details/1428358202592432129

视Linux版本不同，流程会略有不同，但大同小异，这边以5.4.81内核为例。

首先，在启用/proc/net/tcp时，运行了下面的函数：
```C
static int __net_init tcp4_proc_init_net(struct net *net)
{
	if (!proc_create_net_data("tcp", 0444, net->proc_net, &tcp4_seq_ops,
			sizeof(struct tcp_iter_state), &tcp4_seq_afinfo))
		return -ENOMEM;
	return 0;
}
```
其中proc_create_net_data中调用了几个参数，此处，我们看一下函数proc_create_net_data的类型。
```C
struct proc_dir_entry *proc_create_net_data(const char *name, umode_t mode,
		struct proc_dir_entry *parent, const struct seq_operations *ops,
		unsigned int state_size, void *data)
{
	struct proc_dir_entry *p;

	p = proc_create_reg(name, mode, &parent, data);
	if (!p)
		return NULL;
	pde_force_lookup(p);
	p->proc_fops = &proc_net_seq_fops;
	p->seq_ops = ops;
	p->state_size = state_size;
	return proc_register(parent, p);
}
```
该函数本质上就是传参，特别注意，其中proc_dir_entry的类型如下：
```C
/*
 * This is not completely implemented yet. The idea is to
 * create an in-memory tree (like the actual /proc filesystem
 * tree) of these proc_dir_entries, so that we can dynamically
 * add new files to /proc.
 *
 * parent/subdir are used for the directory structure (every /proc file has a
 * parent, but "subdir" is empty for all non-directory entries).
 * subdir_node is used to build the rb tree "subdir" of the parent.
 */
struct proc_dir_entry {
	...
	union {
		const struct seq_operations *seq_ops;
		int (*single_show)(struct seq_file *, void *);
	};
	proc_write_t write;
	void *data;
	unsigned int state_size;
	unsigned int low_ino;
	nlink_t nlink;
	kuid_t uid;
	kgid_t gid;
	loff_t size;
	struct proc_dir_entry *parent;
	...
} __randomize_layout;
```
该数据结构存在seq_operations用于存放函数，其主要功能是在proc目录下创建一个文件夹，根据前文我们可知其将生成文件夹/tcp，且该文件夹在文件系统中按照linux惯有的树结构进行存储，并且seq_operations中存放了处理该文件夹下的一些seq函数。

特别的，我们注意到proc_create_net_data函数中将tcp4_seq_ops系列函数赋值给了他，而这个函数如下所示：
```C
static const struct seq_operations tcp4_seq_ops = {
	.show		= tcp4_seq_show,
	.start		= tcp_seq_start,
	.next		= tcp_seq_next,
	.stop		= tcp_seq_stop,
};
```
我们暂时还尚未得知相关函数何时被启用，先记在这边，往下追溯。

前述分析proc_create_net_data函数时，名为tcp4_seq_afinfo的信息同样值得关注，其被用在proc_create_net_data函数的data载荷之中。简要解析一下该数据结构：该数据结构是tcp_seq_afinfo在tcp4下的一个表现，而tcp_seq_afinfo的信息则相对匮乏。
```C
struct tcp_seq_afinfo {
	sa_family_t			family;
};
```
这也是老版本和新版本内核的不同：在老版本中，相关的函数赋值和寄存器赋值均在tcp_seq_afinfo中进行封装，而新版本则直接将其载荷以seq_op形式在母函数proc_create_net_data中进行传递。

因此，我们继续追溯proc_create_net_data中的函数信息。

调用链如下：
```shell
proc_create_net_data -> proc_register -> pde_subdir_insert插入新文件夹
```
seq_op系列函数被存放在proc_register中的struct proc_dir_entry *dp中，可以注意一下。

至此，我们大致知道了在/tcp下建立这么多项的基本流程，下面我们来看看cat和seq_op是怎么联系在一起的。
### 运行cat /tcp的流程是怎样的？
这边可以取一个巧，我直接strace cat然后不加东西。
```shell
strace cat
```
会发现这个东西会在read函数里卡住，那么好，我们可以去看read函数的系统调用再梳理一下。

```shell
SYSCALL_DEFINE3(read...) -> ksys_read(fd, buf, count) -> vfs_read -> __vfs_read
```
__vfs_read函数详情如下所示：
```C
ssize_t __vfs_read(struct file *file, char __user *buf, size_t count,
		   loff_t *pos)
{
	if (file->f_op->read)
		return file->f_op->read(file, buf, count, pos);
	else if (file->f_op->read_iter)
		return new_sync_read(file, buf, count, pos);
	else
		return -EINVAL;
}
```
可以见到，如果读取的file他自己有用于read的函数，则会优先采用其自身的函数进行读取，而在/tcp下的文件如上所述当是为struct proc_dir_entry类型，那么如果去寻找其read函数，我们实际找到的是这个东西：
```C
proc_dir_entry->proc_net_seq_fops->read = seq_read
```
我们对这个函数做解析（其实从注释里我们也能看出这个函数被用于file->f_op->read在proc_dir_entry的实例，但是鄙人确实也不知道这个东西是怎么实例化的，因为感觉似乎有面向对象编程的意思在，可是我不知道怎么做到的，以后再查吧）
```C
/**
 *	seq_read -	->read() method for sequential files.
 *	@file: the file to read from
 *	@buf: the buffer to read to
 *	@size: the maximum number of bytes to read
 *	@ppos: the current position in the file
 *
 *	Ready-made ->f_op->read()
 */
ssize_t seq_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	//用于实例化的可能用的是这个吧，虽然还不是很清楚
	struct seq_file *m = file->private_data;
	.....
	/* grab buffer if we didn't have one */
	if (!m->buf) {
		m->buf = seq_buf_alloc(m->size = PAGE_SIZE);
		if (!m->buf)
			goto Enomem;
	}
	/* if not empty - flush it first */
	if (m->count) {
		n = min(m->count, size);
		err = copy_to_user(buf, m->buf + m->from, n);
		if (err)
			goto Efault;
		m->count -= n;
		m->from += n;
		size -= n;
		buf += n;
		copied += n;
		if (!size)
			goto Done;
	}
	/* we need at least one record in buffer */
	m->from = 0;
	p = m->op->start(m, &m->index);
	while (1) {
		err = PTR_ERR(p);
		if (!p || IS_ERR(p))
			break;
		err = m->op->show(m, p);//NOTICE！注意这一行
		if (err < 0)
			break;
		if (unlikely(err))
			m->count = 0;
		if (unlikely(!m->count)) {
			p = m->op->next(m, p, &m->index);
			continue;
		}
		if (m->count < m->size)
			goto Fill;
		m->op->stop(m, p);
		kvfree(m->buf);
		m->count = 0;
		m->buf = seq_buf_alloc(m->size <<= 1);
		if (!m->buf)
			goto Enomem;
		m->version = 0;
		p = m->op->start(m, &m->index);
	}
	m->op->stop(m, p);
	m->count = 0;
	goto Done;
Fill:
	/* they want more? let's try to get some more */
	while (1) {
		size_t offs = m->count;
		loff_t pos = m->index;

		p = m->op->next(m, p, &m->index);
		if (pos == m->index)
			/* Buggy ->next function */
			m->index++;
		if (!p || IS_ERR(p)) {
			err = PTR_ERR(p);
			break;
		}
		if (m->count >= size)
			break;
		err = m->op->show(m, p);
		if (seq_has_overflowed(m) || err) {
			m->count = offs;
			if (likely(err <= 0))
				break;
		}
	}
	m->op->stop(m, p);
	n = min(m->count, size);
	err = copy_to_user(buf, m->buf, n);
	if (err)
		goto Efault;
	copied += n;
	m->count -= n;
	m->from = n;
Done:
	if (!copied)
		copied = err;
	else {
		*ppos += copied;
		m->read_pos += copied;
	}
	file->f_version = m->version;
	mutex_unlock(&m->lock);
	return copied;
Enomem:
	err = -ENOMEM;
	goto Done;
Efault:
	err = -EFAULT;
	goto Done;
}
```
见注释含有中文的一行，我们确乎是看到show函数被调用了。那么这么一来，我们总算知道了seq_show函数的调用流程。

接下来就是挂钩的事情了，wiki的补充就简写到这。