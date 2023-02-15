#include "ctxt_switch.h"

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>


LIST_HEAD(pid_head);


long __register(pid_t pid) {
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

long __fetch(struct pid_ctxt_switch * stats) {
    struct pid_node *cursor;
    struct task_struct *t;

    stats->ninvctxt = 0;
    stats->nvctxt = 0;

    list_for_each_entry(cursor, &pid_head, next_prev_list) {
        t = find_task_by_vpid(cursor -> pid);
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

long __deregister(pid_t pid) {
    struct pid_node *cursor, *temp;
    
    if (pid < 1) {
        return -22;
    }

    list_for_each_entry_safe(cursor, temp, &pid_head, next_prev_list) {
        if (cursor -> pid == pid) {
            list_del(&cursor->next_prev_list);
            kfree(cursor);
            return 0;
        }
    }

    return -3;
}


SYSCALL_DEFINE1(register, pid_t, pid) {
    return __register(pid);
}

SYSCALL_DEFINE1(fetch, struct pid_ctxt_switch *, stats) {
    return __fetch(stats);
}

SYSCALL_DEFINE1(deregister, pid_t, pid) {
    return __deregister(pid);
}
