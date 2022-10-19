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

## 

参考资料：


https://cdn.modb.pro/db/415873
