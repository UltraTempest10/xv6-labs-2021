#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int
readline(char *buffer, int size)
{
    int i = 0;
    char c;

    while (i < size - 1) {
        if (read(0, &c, 1) != 1) {
            return 0;    // Read error or end of file
        }

        buffer[i++] = c;

        if (c == '\n') {
            break;    // Stop at newline
        }
    }

    buffer[i - 1] = '\0';    // Remove '\n'
    return i - 1;
}

int
main(int argc, char *argv[])
{
    int num_args = 0;
    char *args[MAXARG + 1];
    for (int i = 1; i < argc; i++) {
        args[num_args++] = argv[i];
    }

    // Collect arguments from standard input
    char buffer[128];
    while (num_args < MAXARG && readline(buffer, sizeof(buffer))) {
        if (buffer[0] != '\0') {

            // printf("read arg: %s\n", buffer);

            char *arg = malloc(strlen(buffer) + 1);
            strcpy(arg, buffer);
            args[num_args++] = arg;
        }
    }

    args[num_args] = 0;
    int c = fork();
    if (c > 0) {
        wait((int *) 0);
    }
    else if (c == 0) {
        exec(argv[1], args);
        exit(0);
    }
    else {
        printf("fork error\n");
    }

    for (int i = 0; i < num_args; i++) {
        free(args[i]);
    }
    exit(0);
}