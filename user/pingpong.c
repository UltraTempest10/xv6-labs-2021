#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int p[2];
    char str[10];
    pipe(p);  
    write(p[1], "ping", 4);
    
    int c = fork();
    if (c > 0) {
        read(p[0], str, sizeof(str));
        printf("%d: received %s\n", c, str);
        write(p[1], "pong", 4);
        c = wait((int *) 0);
    } 
    else if (c == 0) {
        exit(0);
    } 
    else {
        printf("fork error\n");
    }

    read(p[0], str, sizeof(str));
    printf("%d: received %s\n", getpid(), str);

    exit(0);
}