## Notification背景知识
to fulfill the need for interaction, Linux uses so called notification chains.

for each notification chain there is a passive side(notified) and an active side(notifier).
- the notified are the subsystems that ask to be notified about the event and that provide a callback function to invoke.
- the notifier is the subsystem that experiences n event and calls the callback function.

```C
typedef	int (*notifier_fn_t)(struct notifier_block *nb, unsigned long action, void *data);

struct notifier_block {
	notifier_fn_t notifier_call;
	struct notifier_block __rcu *next;
	int priority;
};
```
看得出来，这是一个链表来存储的结构。

When a kernel component is interested in the events of a given notification chain, it can register it with the general function notifier_chain_register and to unregister we can use the function notifier_chain_unregister.

注意到上面的notifier_block之中存放了一个priority，或许这是一个决定优先级的变量，我们结合notifier_block模块的注册流程来看一下：
```C
static int notifier_chain_register(struct notifier_block **nl,
		struct notifier_block *n)
{
	while ((*nl) != NULL) {
		if (unlikely((*nl) == n)) {
			WARN(1, "double register detected");
			return 0;
		}
		if (n->priority > (*nl)->priority)
			break;
		nl = &((*nl)->next);
	}
	n->next = *nl;
	rcu_assign_pointer(*nl, n);
	return 0;
}
```
所有的notifier_block将会在注册时塞进一个优先级队列之中，通过比较各自的优先级决定谁先谁后。谁的优先级高，谁就在前面。

同理，我们可以将其注册取消掉：
```C
static int notifier_chain_unregister(struct notifier_block **nl,
		struct notifier_block *n)
{
	while ((*nl) != NULL) {
		if ((*nl) == n) {
			rcu_assign_pointer(*nl, n->next);
			return 0;
		}
		nl = &((*nl)->next);
	}
	return -ENOENT;
}
```
rcu_assign_pointer显然是一个链表操作赋值函数，估计功能很简单，就不再解释。

注册的时候，需要利用notifier_call_chain来帮助产生notifications。
```C
static int notifier_call_chain(struct notifier_block **nl,
			       unsigned long val, void *v,
			       int nr_to_call, int *nr_calls)
{
	int ret = NOTIFY_DONE;
	struct notifier_block *nb, *next_nb;

	nb = rcu_dereference_raw(*nl);

	while (nb && nr_to_call) {
		next_nb = rcu_dereference_raw(nb->next);

#ifdef CONFIG_DEBUG_NOTIFIERS
		if (unlikely(!func_ptr_is_kernel_text(nb->notifier_call))) {
			WARN(1, "Invalid notifier called!");
			nb = next_nb;
			continue;
		}
#endif
		ret = nb->notifier_call(nb, val, v);

		if (nr_calls)
			(*nr_calls)++;

		if (ret & NOTIFY_STOP_MASK)
			break;
		nb = next_nb;
		nr_to_call--;
	}
	return ret;
}
```
特别的，上述函数较为底层（毕竟涉及了优先级队列这个已经到实现层面的东西了），真实情况下对于模块的插装信息，采用下面的函数进行注册：
```C
int register_module_notifier(struct notifier_block *nb);

int unregister_module_notifier(struct notifier_block *nb);
```
阅读源码可知这两个函数确实调用了上面所述的相关注册和注册取消函数。

## 内核模块的加载与notification的联系
```C
SYSCALL_DEFINE3(init_module, void __user *, umod, unsigned long, len, const char __user *, uargs)
{
	int err;
	struct load_info info = { };

	err = may_init_module();
	if (err)
		return err;

	pr_debug("init_module: umod=%p, len=%lu, uargs=%p\n",
	       umod, len, uargs);

	err = copy_module_from_user(umod, len, &info);
	if (err)
		return err;

	return load_module(&info, uargs, 0);
}
```
syscall_define3 (test for whether got enough spaces & copy) -> load_module
```C
static int load_module(struct load_info *info, const char __user *uargs, int flags)
{
    // elf_header_check
    // setup_load_info ... many checks.
    .....
    /* Finally it's fully formed, ready to start executing. */
	err = complete_formation(mod, info);
	if (err)
		goto ddebug_cleanup;

	err = prepare_coming_module(mod);
	if (err)
		goto bug_cleanup;
    .....
    // execute.
    return do_init_module(mod);
}
```
跟着source code一步一步走，函数的调用链如下所示(5.4.81)：
```
prepare_coming_module -> blocking_notifier_call_chain -> __blocking_notifier_call_chain -> notifier_call_chain
```
(确实是这样，我没有骗人)。

## 源码分析的参数传递：
源码分析在告知上述流程的同时，除了了解notifer_call发挥的作用以外，我们还需要注意部分特殊参数的传递。

在SYSCALL_DEFINE3中，我们调用了copy_module_from_user函数，通过分析该函数可知，在LOADING_MODULE环节下，我们会将load_info存放到info之中，利用相关flag和该info信息，我们在load_module函数中调用了layout_and_allocate函数创建一个mod，而该mod的数据结构类型为module，大家可以自行查阅。

这个mod如上文所述被用在了prepare_coming_module函数之中，请注意其被调用的参数的位置：
```C
static int prepare_coming_module(struct module *mod)
{
	int err;

	ftrace_module_enable(mod);
	err = klp_module_coming(mod);
	if (err)
		return err;

	blocking_notifier_call_chain(&module_notify_list,
				     MODULE_STATE_COMING, mod);
	return 0;
}
```
是不是感觉世界线好像收束了？没错，mod信息在blocking_notifier_call_chain中是作为第三个参数出现的，如果还不太明白的话，我们看看接下来紧跟的几个函数。
```C
__blocking_notifier_call_chain(nh, val, v, -1, NULL); // 参数v即为mod存储的位置

notifier_call_chain(&nh->head, val, v, nr_to_call, nr_calls); // 参数v即为mod存储的位置

ret = nb->notifier_call(nb, val, v); // 参数v即为mod存储的位置
```
再看看fake.c中的关键函数行：
```C
int my_notifier_call(struct notifier_block *nb, unsigned long action, void *data);
```
所以呢，当我们自己定义了一个notifier_call函数并插装到自己写的模块之中，不出意外notifier_call_chain会把包含新模块装载的信息以参数v来传递，这就是我们的void * data部分。

总而言之，当我们的fake模块装载完后，再装一个test模块，其信息会以info的形式传递，并生成module信息，这个module信息将会在notifier_call链中传递，直到notifier_call函数使用了这个信息。

这样一来，源码分析最大头的部分：
```C
module = data;
```
也就顺理解决了，开香槟！

有了module信息，偷天换日其init和exit函数也不是什么难事，修改一下函数指针就好了！
## 实验结果：

```shell
root@ubuntu:/home/link/Desktop/libre/fake/fake# dmesg
[ 5107.818791] Rootkit is entering.
[ 5107.818806] Processing the module: fake
[ 5115.706725] Processing the module: test
[ 5115.706725] Replacing init and exit functions: test.
[ 5115.706761] fake_init lanuched.
[ 5115.706761] Processing the module: test
[ 5125.442091] fake_exit ended
[ 5125.442092] Processing the module: test
[ 5127.655346] Rootkit is leaving.
```
