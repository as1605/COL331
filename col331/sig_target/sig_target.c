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

#define AUTHOR "Aditya Singh <ee1200461@iitd.ac.in>"
#define PROCFS_MAX_SIZE 1024
#define PROCFS_NAME "sig_target"
#define PERMS 0666
#define WORK_QUEUE_NAME "WQsig_target.c"
#define WQ_TIMER_MS 1000 /* 1000 ms */

MODULE_AUTHOR(AUTHOR);

static int timer_interrupt_count = 0;

static struct workqueue_struct *my_workqueue = NULL;
static struct work_struct work;
// static DECLARE_WORK(Task, intrpt_routine, NULL);

static struct proc_dir_entry *Sig_Target;
static char procfs_buffer[PROCFS_MAX_SIZE];
static unsigned long procfs_buffer_size = 0;

static void work_handler(struct work_struct * work)
{
	timer_interrupt_count++;

	// TODO: remove after debugging
	printk(KERN_ALERT "routine %d\n %s", timer_interrupt_count,
	       procfs_buffer);

	siginfo_t info;
	memset(&info, 1, sizeof(info));

	char *ptr = procfs_buffer;
	while (ptr) {
		int p, s;
		sscanf(ptr, "%d, %d", &p, &s);
		pid_t pid = p;
		struct task_struct *task = find_task_by_vpid(pid);
		int ret = send_sig_info(s, &info, task);
		// TODO: remove after debugging
		printk(KERN_ALERT
		       "sent signal %d to process %d and returned %d",
		       s, p, ret);
		ptr = strchr(ptr, '\n');
	}

	msleep(WQ_TIMER_MS);
}

static ssize_t procfile_read(
		struct file *file, char __user *buffer, size_t count, loff_t *ppos
		)
{
	int ret;

	printk(KERN_INFO "procfile_read (/proc/%s) called\n", PROCFS_NAME);

	if (*ppos > 0) {
		ret = 0;
	} else {
		copy_to_user(buffer, procfs_buffer, procfs_buffer_size);
		ret = procfs_buffer_size;
	}

	return ret;
}

static ssize_t procfile_write(
		struct file *file, const char __user *buffer,size_t count, loff_t *ppos)
{
	procfs_buffer_size = count;
	if (procfs_buffer_size > PROCFS_MAX_SIZE) {
		procfs_buffer_size = PROCFS_MAX_SIZE;
	}

	if (copy_from_user(procfs_buffer, buffer, procfs_buffer_size)) {
		return -EFAULT;
	}

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
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n", PROCFS_NAME);
		return -ENOMEM;
	}

	printk(KERN_INFO "/proc/%s created\n", PROCFS_NAME);

	my_workqueue = alloc_workqueue(WORK_QUEUE_NAME, WQ_UNBOUND, 1);
	INIT_WORK(&work, work_handler); 
    schedule_work(&work); 
	return 0;
}

void cleanup_module()
{
	proc_remove(Sig_Target);
	printk(KERN_INFO "/proc/%s removed\n", PROCFS_NAME);
}
