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
## 内核模块的加载与notification的联系
