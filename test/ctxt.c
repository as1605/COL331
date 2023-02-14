#include <col331/ctxt_switch.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

int main() {
    printf("Hello\n");
    struct pid_ctxt_switch * s = malloc(sizeof(struct pid_ctxt_switch));
    pid_t p;
    
    p = getpid();
    printf("PID is %d", &p);
    
    long int ret;
    
    ret = syscall(452, p);
    printf("register system call returned %ld\n", ret);
    
    ret = syscall(453, s);
    printf("fetch system call returned %ld\n", ret);
    printf("ninvctxt=%ld\t nvctxt=%ld\n", s->ninvctxt, s->nvctxt);
    
    ret = syscall(453, s);
    printf("fetch system call returned %ld\n", ret);
    printf("ninvctxt=%ld\t nvctxt=%ld\n", s->ninvctxt, s->nvctxt);
    
    ret = syscall(452, p);
    printf("register system call returned %ld\n", ret);
    
    ret = syscall(453, s);
    printf("fetch system call returned %ld\n", ret);
    printf("ninvctxt=%ld\t nvctxt=%ld\n", s->ninvctxt, s->nvctxt);
    
    ret = syscall(454, p);
    printf("deregister system call returned %ld\n", ret);
    
    ret = syscall(453, s);
    printf("fetch system call returned %ld\n", ret);
    printf("ninvctxt=%ld\t nvctxt=%ld\n", s->ninvctxt, s->nvctxt);
    
    return 0;
}