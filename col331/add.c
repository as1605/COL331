#include <linux/kernel.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE2(add, int, a, int, b) {
    printk(KERN_ALERT "Sum is %d\n", a+b);
    return 0;
}

