#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

int main() {
    printf("Enter 2 numbers\n");
    int a, b;
    scanf("%d %d", &a, &b);
    long int amma = syscall(451, a ,b);
    printf("System call returned %ld\n", amma);
    return 0;
}