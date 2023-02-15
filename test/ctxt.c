#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>

struct pid_ctxt_switch {
	unsigned long ninvctxt; /* Count of involuntary context switches */
	unsigned long nvctxt; /* Count of voluntary context switches*/
};

void foo()
{
	for (int i = 0; i < 10000000; i++)
		;
	sleep(1);
}

int main()
{
	printf("Hello World!\n");
	struct pid_ctxt_switch *s = malloc(sizeof(struct pid_ctxt_switch));
	pid_t p;

	p = getpid();
	printf("PID is %d\n", p);

	long int ret;

	ret = syscall(452, p);
	printf("register system call returned %ld\n", ret);
	ret = syscall(453, s);
	printf("fetch system call returned %ld\n", ret);
	printf("ninvctxt=%ld\t nvctxt=%ld\n", s->ninvctxt, s->nvctxt);
	foo();

	foo();
	ret = syscall(453, s);
	printf("fetch system call returned %ld\n", ret);
	printf("ninvctxt=%ld\t nvctxt=%ld\n", s->ninvctxt, s->nvctxt);

	foo();

	ret = syscall(453, s);
	printf("fetch system call returned %ld\n", ret);
	printf("ninvctxt=%ld\t nvctxt=%ld\n", s->ninvctxt, s->nvctxt);

	foo();

	ret = syscall(452, p);
	printf("register system call returned %ld\n", ret);

	foo();

	ret = syscall(453, s);
	printf("fetch system call returned %ld\n", ret);
	printf("ninvctxt=%ld\t nvctxt=%ld\n", s->ninvctxt, s->nvctxt);

	foo();

	ret = syscall(454, p);
	printf("deregister system call returned %ld\n", ret);
	foo();

	foo();

	ret = syscall(453, s);
	printf("fetch system call returned %ld\n", ret);
	printf("ninvctxt=%ld\t nvctxt=%ld\n", s->ninvctxt, s->nvctxt);

	return 0;
}
