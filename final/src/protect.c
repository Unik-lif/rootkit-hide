#include "../header/hide.h"
/* -------------------------------------- *
 *              其余工具                   *
 * -------------------------------------- */

// 写保护
void write_cr0_forced(unsigned long val) {
    unsigned long __force_order;

    asm volatile(
        "mov %0, %%cr0"
        : "+r"(val), "+m"(__force_order));
    // 以防编译优化而添加dummy部分（__force_order）
}

// 翻转cr0的第十六位内容以关闭或者开启写保护
void unprotect_memory(void) {
    write_cr0_forced(read_cr0() & (~ 0x10000));
}

void protect_memory(void) {
    write_cr0_forced(read_cr0() | (0x10000));
}