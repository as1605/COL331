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
