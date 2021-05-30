#include "user.h"

void test(char *argc, char *argv[]) {
    int pid = fork();
    if (pid == 0) {
        execve(argc, argv, NULL);
    } else {
        wait(0);
    }
}


void main(){
    open("dev/tty", O_RDWR);
    dup(0);
    dup(0);
    test("/getpid", NULL);
    test("/getppid", NULL);
    test("/getcwd", NULL);
    test("/fork", NULL);
    test("/exit", NULL);
    test("/mkdir_", NULL);
    test("/dup", NULL);
    test("/write", NULL);
    test("/read", NULL);
    test("/open", NULL);
    test("/wait", NULL);
    test("/waitpid", NULL);
    test("/yield", NULL);
    test("/openat", NULL);
    test("/close", NULL);
  // test("/clone", NULL);
    test("/chdir", NULL);
    test("/execve", NULL);
    test("/dup2", NULL);
    test("/brk", NULL);
    test("/uname", NULL);
    test("/pipe", NULL);
    test("/getdents", NULL);
    test("/mount", NULL);
    test("/umount", NULL);
    test("/times", NULL);
    test("/gettimeofday", NULL);
    test("/mmap", NULL);
    test("/munmap", NULL);
    test("/fstat", NULL);
    test("/unlink", NULL);
    test("/clone", NULL);
    while (1) {
    };
}
