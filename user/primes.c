#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
prime(int p[2])
{
    int n, np[2];
    if (read(p[0], &n, sizeof(n)) == 0) {
        exit(0);
    }
    printf("prime %d\n", n);
    
    pipe(np);
    int nc = fork();
    if (nc > 0) {
        close(np[1]);
        prime(np);
    }
    else if (nc == 0) {
        close(np[0]);
        int m;
        while (read(p[0], &m, sizeof(m)) == sizeof(m)) {
            if (m % n != 0) {
                write(np[1], &m, sizeof(m));
            }
        }
        close(np[1]);
        exit(0);
    }
    else {
        printf("fork error\n");
    }
}

int
main(int argc, char *argv[])
{
    int n, p[2];
    pipe(p);

    for (n = 2; n < 36; n++) {
        write(p[1], &n, sizeof(n));
    }
    close(p[1]);
    
    int c = fork();
    if (c > 0) {
        ;
    }
    else if (c == 0) {
        prime(p);
        exit(0);
    }
    else {
        printf("fork error\n");
    }
    close(p[0]);
    wait((int *) 0);

    exit(0);
}