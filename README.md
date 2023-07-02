# Introduction
[Assignment 1](https://www.cse.iitd.ac.in/~srsarangi/courses/2023/col_331_2023/hw/OS_A1_hard.pdf)  of [Prof. Smruti R. Sarangi](https://www.youtube.com/@srsarangi)'s [COL331](https://www.cse.iitd.ac.in/~srsarangi/courses/2023/col_331_2023/index.html) course on Operating Systems in 2023. 

Modifying the Linux kernel (v6.1.6) to add a system call for track context switches and a kernel module to generate signals to any process.

# Install Linux

## Machine

I usually use an M1 MacBook Air for my work, but I realised there would
be many complications on it so I switched to my old Windows laptop for
this assignment, it runs an i3-5005U with 8GB RAM.

Rather than using the `virt-manager` of Ubuntu, I decided to use
`Oracle VM VirtualBox` as I’m more comfortable with it and it runs on
Windows easily. On the VM, I installed
`ubuntu-22.04.1-live-server-amd64.iso` with the default options.

For the storage, I had to extend the default partition using `resize2fs`
and `lvextend` commands.

## Source

Earlier I had avoided cloning the repo and downloaded the tar of the
`v6.1.6` directly. However, later I decided to maintain the source on my
private repo GitHub, as it would be easier to maintain and also easier
to generate the **patch**, and also reserve the original commits on the
source tree.

It was not possible to fork `torvalds/linux` as it would not be a
private repo, so I decided to first clone the original repo (tag v6.1.6)
from git.kernel.org link and then push it to my GitHub using ssh key.
However, the pack size was almost around 4GB so I had to use
`git gc –aggressive` to bring it down to around 1.8GB as the upper limit
by GitHub is 2GB.

By such an arrangement, now I can code on my MacBook and then push to
GitHub then pull on the VM to run and also push back any changes from
the VM. I also have the flexibility of switching to some other VM in
future.

## Config

In the `.config` file, I also had to blank out the
`CONFIG_SYSTEM_REVOCATION_KEYS` entry apart from the given
`CONFIG_SYSTEM_TRUSTED_KEYS` entry in the instructions. In the options
after running `make` for the first time (or `make menuconfig` I pressed
ENTER all through to use the default options.

## Installation

On running `make -j4` then `sudo make modules_install -j4` then
`sudo make install -j4` and `sudo update-grub` the kernel was
successfully installed.

The `make` step took a very long time (around 10-12 hours) as the
machine I was using had only 2 cores and no SSD.

<img src="img/A1.JPG" style="width:50.0%" alt="image" />
<img src="img/A2.JPG" style="width:50.0%" alt="image" />

# Context Switch Tracker

The key observation was that in the `task_struct` there were fields for
`nvcsw` and `nivcsw` which automatically stored the context switch
counts for that pid, we now just had to sum over the individual PID
which are stored in the linked list. We can get the `task_struct` using
`find_task_by_vpid` function, where the vpid ensures that the task does
not refer a pid beyond its namespace.

## System Call

For learning how to create a signal call, I first practiced by creating
a program `add.c` for just adding 2 numbers. I made a folder `col331/`
in the directory, added a Makefile with `obj-y := add.o` to tell it to
compile that file and in the main Makefile I put `core-y := col331/` to
tell it to include that folder while compiling the main kernel.

To register the system call, I used `SYSCALL_DEFINEn` syntax (which
expands to `asmlinkage` in the macros). Then added a prototype in
`include/linux/syscalls.h`, defined its syscall number in
`include/uapi/asm-generic/unistd.h` and added it to the syscall table at
`arch/x86/entry/syscalls/syscall_64.tbl`

Finally I created a test file in `test/add.c` which used `syscall()`
function to see if the system call is working fine. For logging I had
used `KERN_ALERT` so the message would display in the console (which had
the default log level set to 4).

Once I got this working, similarly I defined functions for register,
fetch, deregister and declared their syscalls as 452, 453 and 454
respectively.

## Linked List

I studied the list mechanism from the book "Linux Kernel Development" by
Robert Love. So first we define the structure for pid_node as given in
the assignment, and then we declare the head of the list using
`LIST_HEAD(pid_head);` macro

In the register function, we first create a pid node using `kmalloc`
(instead of malloc as we need to allocate in kernel space). Then we use
`INIT_LIST_HEAD` macro to initialise the list head field, and then add
it to the end of the linked list using `list_add_tail` function

In the fetch function, we first set the counts in the struct to 0, and
then we traverse the list using `list_for_each_entry` function and sum
up the context switch counts.

In the deregister function, we have to use `list_for_each_entry_safe`
instead as it is safe against deletion of elements in between (although
we need to delete only once so this should not be much of an issue)

We also use `find_task_by_vpid` function to check if the pid is valid or
not.

In the implementation, to avoid messing up the syntax with the
`SYSCALL_DEFINE` macros, I first defined the functions in the regular
way but with `__` prefix then called them inside the function defined
with `SYSCALL_DEFINE` like a wrapper.

For the testing, I created a dummy function `foo()` which runs a loop
for 10<sup>8</sup> times then sleeps for 1 second, so we have both
involuntary and voluntary context switches for that process.

<img src="img/A3.JPG" alt="image" />

# On Demand Signal Generator using a Kernel Module

## Setup: Module

For creating a module, first I created a directory `col331/sig_target`,
initialised `sig_target.c`. We need `int init_module()` and
`void cleanup_module()` functions to define the insertion and removal of
the module in the kernel. They can be renamed but I followed the
convention.

In the Makefile I added `obj-m += sig_target.o` to tell it to compile
and `all: make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules`
for adding the module.

The module is compiled with `make`, then we use
`sudo insmod sig_target.ko` to insert the module into the kernel and
`sudo rmmod sig_target` to remove it.

## Input: /proc/sig_target "file"

The proc filesystem allows us to take input and output from the user
space. The "file" is stored in kernel space memory as a 1024 character
array `procfs_buffer[PROCFS_MAX_SIZE]`.

It can be written to and read by the user by using the
`/proc/sig_target` "file", so we use 0666 permission (altho it was not
necessary for the user to *read* the file technically as per the
assignment task, but it was needed for convenience)

We declare a `proc_dir_entry` which is initialised along with the
module. When the user reads the file, `proc_read` is invoked (which
basically copies from the kernel space buffer to the user space buffer),
and when they write to it, `proc_write` is invoked (which basically
copies from the user space buffer to the kernel space buffer). A pointer
is maintained to keep the offset till where the buffer has been written
to or read.

This proc file is deleted when the module is cleaned up.

For testing, we should create a `tmp` file for ease of editing, then use
command `cat tmp > /proc/sig_target` to write to the file, and
`cat /proc/sig_target` to read it.

## Timer: Work Queue

We need to check the proc file **every second**, so I used the
`workqueue` mechanism. I assumed
$1$ second $= 1000$ jiffies but it can be set
according to the machine on which it is actually running.

When the module is initialised, we first allocate a work queue and
initialise the work with `INIT_DELAYED_WORK` macro. Then we call
`schedule_delayed_work` in a recursive fashion

In the worker function, we basically read line by line using `strchr`
from `linux/string.h` then extract the pid and signal using `sscanf`
function

## Signal: send_sig_info

We get the pointer to the pid struct using `find_vpid` function, it is
0 if the pid in proc file is invalid. If it is valid we fetch the
`task_struct` using `pid_task` function. Finally, we send the signal to
that task using `send_sig_info` function, keeping our access as kernel
in the info.

While testing, we can try sending signal 9 (SIGKILL) to processes and
see if they’re killed.

<img src="img/A4.JPG" alt="image" />

# Appendix

The source files have been formatted with clang-format. The diff was
generated using
`sudo diff -rupN linux-base/ linux-change/ > ctxttrack.patch` instead of
`sudo diff -rupN linux-change/ linux-base/ > ctxttrack.patch`. I have
also supplied a git patch generated with
`git diff 38f3ee1 HEAD > ctxttrack.git.patch`

## Snippets

### ctx_switch.h

``` c
#include <linux/types.h>
#include <linux/list.h>

struct pid_node {
    pid_t pid; /* process id */
    struct list_head
        next_prev_list; /* contains pointers to previous and next elements */
};

struct pid_ctxt_switch {
    unsigned long ninvctxt; /* Count of involuntary context switches */
    unsigned long nvctxt; /* Count of voluntary context switches*/
};
```

### ctx_switch.c

``` c
#include "ctxt_switch.h"

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>

LIST_HEAD(pid_head);

long __register(pid_t pid)
{
    struct task_struct *t;
    struct pid_node *temp_node;

    if (pid < 1) {
        return -22;
    }

    t = find_task_by_vpid(pid);
    if (!t) {
        return -3;
    }

    temp_node = kmalloc(sizeof(struct pid_node), GFP_KERNEL);
    temp_node->pid = pid;
    INIT_LIST_HEAD(&temp_node->next_prev_list);
    list_add_tail(&temp_node->next_prev_list, &pid_head);

    return 0;
}

long __fetch(struct pid_ctxt_switch *stats)
{
    struct pid_node *cursor;
    struct task_struct *t;

    stats->ninvctxt = 0;
    stats->nvctxt = 0;

    list_for_each_entry(cursor, &pid_head, next_prev_list) {
        t = find_task_by_vpid(cursor->pid);
        if (!t) {
            list_del(&cursor->next_prev_list);
            kfree(cursor);
            return -22;
        }
        stats->ninvctxt += t->nivcsw;
        stats->nvctxt += t->nvcsw;
    }

    return 0;
}

long __deregister(pid_t pid)
{
    struct pid_node *cursor, *temp;

    if (pid < 1) {
        return -22;
    }

    list_for_each_entry_safe(cursor, temp, &pid_head, next_prev_list) {
        if (cursor->pid == pid) {
            list_del(&cursor->next_prev_list);
            kfree(cursor);
            return 0;
        }
    }

    return -3;
}

SYSCALL_DEFINE1(register, pid_t, pid)
{
    return __register(pid);
}

SYSCALL_DEFINE1(fetch, struct pid_ctxt_switch *, stats)
{
    return __fetch(stats);
}

SYSCALL_DEFINE1(deregister, pid_t, pid)
{
    return __deregister(pid);
}
```

### sig_target.c

``` c
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <linux/sched/signal.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/pid.h>

#define AUTHOR "Aditya Singh <ee1200461@iitd.ac.in>"
#define PROCFS_MAX_SIZE 1024
#define PROCFS_NAME "sig_target"
#define PERMS 0666
#define WORK_QUEUE_NAME "WQsig_target.c"
#define WQ_TIMER_DELAY 1000 /* 1000 jiffy = 1s */

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);

static int timer_interrupt_count = 0;

static struct workqueue_struct *my_workqueue = NULL;
static struct delayed_work work;

static struct proc_dir_entry *Sig_Target;
static char procfs_buffer[PROCFS_MAX_SIZE];
static unsigned long procfs_buffer_size = 0;

static void work_handler(struct work_struct *_work)
{
    struct kernel_siginfo info;
    struct task_struct *task;
    char *ptr = procfs_buffer;
    int p, s, ret;
    pid_t pid;
    struct pid *v;
    timer_interrupt_count++;

    printk(KERN_INFO "routine %d\n %s", timer_interrupt_count,
           procfs_buffer);

    memset(&info, 1, sizeof(info));

    while (ptr && procfs_buffer_size) {
        sscanf(ptr, "%d, %d", &p, &s);
        pid = p;
        printk(KERN_INFO "sending signal %d to process %d", s, p);
        v = find_vpid(pid);
        if (v == 0) {
            printk(KERN_ALERT "Invalid PID %d", v);
        } else {
            printk(KERN_INFO "virtual pid %d", v);
            task = pid_task(v, PIDTYPE_PID);
            ret = send_sig_info(s, &info, task);
            printk(KERN_INFO
                   "sent signal %d to process %d and returned %d",
                   s, p, ret);
        }
        ptr++;
        ptr = strchr(ptr, '\n');
    }

    schedule_delayed_work(&work, WQ_TIMER_DELAY);
}

static ssize_t procfile_read(struct file *file, char __user *buffer,
                 size_t count, loff_t *ppos)
{
    int ret, c;

    printk(KERN_INFO "procfile_read (/proc/%s) called\n", PROCFS_NAME);

    if (*ppos > 0) {
        ret = 0;
    } else {
        c = copy_to_user(buffer, procfs_buffer, procfs_buffer_size);
        ret = procfs_buffer_size;
    }
    *ppos = procfs_buffer_size;
    return ret;
}

static ssize_t procfile_write(struct file *file, const char __user *buffer,
                  size_t count, loff_t *ppos)
{
    printk(KERN_INFO "procfile_write (/proc/%s) called with count %d\n",
           PROCFS_NAME, count);
    procfs_buffer_size = count;

    if (*ppos > 0) {
        return -EFAULT;
    }

    if (procfs_buffer_size > PROCFS_MAX_SIZE - 1) {
        procfs_buffer_size = PROCFS_MAX_SIZE - 1;
    }

    if (copy_from_user(procfs_buffer, buffer, procfs_buffer_size)) {
        return -EFAULT;
    }
    procfs_buffer[procfs_buffer_size] = '\0';

    *ppos = strlen(procfs_buffer);
    return procfs_buffer_size;
}

static struct proc_ops ops = {
    .proc_read = procfile_read,
    .proc_write = procfile_write,
};

int init_module()
{
    Sig_Target = proc_create(PROCFS_NAME, PERMS, NULL, &ops);

    if (Sig_Target == NULL) {
        printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
               PROCFS_NAME);
        return -ENOMEM;
    }
    printk(KERN_ALERT "/proc/%s created\n", PROCFS_NAME);

    my_workqueue = alloc_workqueue(WORK_QUEUE_NAME, WQ_UNBOUND, 1);
    INIT_DELAYED_WORK(&work, work_handler);
    schedule_delayed_work(&work, WQ_TIMER_DELAY);
    return 0;
}

void cleanup_module()
{
    cancel_delayed_work_sync(&work);
    proc_remove(Sig_Target);
    printk(KERN_ALERT "/proc/%s removed\n", PROCFS_NAME);
}
```
