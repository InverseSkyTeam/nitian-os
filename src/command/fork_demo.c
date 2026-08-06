#include "stdio.h"
#include "syscall.h"

int main(void) {
    int32_t pid = fork();
    if (pid > 0) {
        printf("parent: fork returned %d\n", pid);
    } else if (pid == 0) {
        printf("child: fork returned 0, my pid=%d\n", (int)getpid());
    } else {
        printf("fork failed\n");
    }
    exit(0);
    return 0;
}
