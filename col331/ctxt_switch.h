#include <include/linux/types.h>
#include <include/linux/list.h>

struct pid_node { 
    pid_t pid;  /* process id */
    struct list_head next_prev_list; /* contains pointers to previous and next elements */
};

struct pid_ctxt_switch {
    unsigned long ninvctxt; /* Count of involuntary context switches */
    unsigned long nvctxt; /* Count of voluntary context switches*/
}
