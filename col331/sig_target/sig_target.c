#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <linux/sched.h>

#define AUTHOR "Aditya Singh <ee1200461@iitd.ac.in>"
#define PROCFS_MAX_SIZE 1024
#define PROCFS_NAME "sig_target"
#define PERMS 0666
#define WORK_QUEUE_NAME "WQsig_target.c"
#define WQ_TIMER_DELAY 1000 /* 1000 Jiffies = 1 second */

MODULE_AUTHOR(AUTHOR);

static int timer_interrupt_count = 0;
static void intrpt_routine(void *);

static struct workqueue_struct *my_workqueue;
static struct work_struct Task;
static DECLARE_WORK(Task, intrpt_routine, NULL);

static struct proc_dir_entry *Sig_Target;
static char procfs_buffer[PROCFS_MAX_SIZE];
static unsigned long procfs_buffer_size = 0;

static void intrpt_routine(void *)
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

	queue_delayed_work(my_workqueue, &Task, WQ_TIMER_DELAY);
}

int procfile_read(char *buffer, char **buffer_location, off_t offset,
		  int buffer_length, int *eof, void *data)
{
	int ret;

	printk(KERN_INFO "procfile_read (/proc/%s) called\n", PROCFS_NAME);

	if (offset > 0) {
		ret = 0;
	} else {
		memcpy(buffer, procfs_buffer, procfs_buffer_size);
		ret = procfs_buffer_size;
	}

	return ret;
}

int procfile_write(struct file *file, const char *buffer, unsigned long count,
		   void *data)
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

int init_module()
{
	Sig_Target = create_proc_entry(PROCFS_NAME, PERMS, NULL);

	if (Sig_Target == NULL) {
		remove_proc_entry(PROCFS_NAME, &proc_root);
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
		       PROCFS_NAME);
		return -ENOMEM;
	}

	Sig_Target->read_proc = procfile_read;
	Sig_Target->write_proc = procfile_write;
	Sig_Target->owner = THIS_MODULE;
	Sig_Target->mode = S_IFREG | S_IRUGO;
	Sig_Target->uid = 0;
	Sig_Target->gid = 0;
	Sig_Target->size = 37;

	printk(KERN_INFO "/proc/%s created\n", PROCFS_NAME);
	return 0;
}

void cleanup_module()
{
	remove_proc_entry(PROCFS_NAME, &proc_root);
	printk(KERN_INFO "/proc/%s removed\n", PROCFS_NAME);
}
