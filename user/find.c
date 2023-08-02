#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
getname(char *path)
{
    char *p;

    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return name.
    return p;
}

void
find(char *path, char *name)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type) {
        case T_FILE:
            if(strcmp(getname(path), name) == 0) {
                printf("%s\n", path);
            }
            break;

        case T_DIR:
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
                printf("find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)) {

                // printf("de.name: %s\n", de.name);

                if(de.inum == 0) {
                    continue;
                }
                if (!strcmp(de.name, ".") || !strcmp(de.name, ".."))
                {

                    // printf("skip,%s\n", de.name);
                    
                    continue;
                }
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if(stat(buf, &st) < 0) {
                    printf("find: cannot stat %s\n", buf);
                    continue;
                }
                if(st.type == T_FILE && strcmp(de.name, name) == 0) {
                    printf("%s\n", buf);
                }
                else if(st.type == T_DIR) {
                    find(buf, name);
                }
            }
            break;
    }
}

int
main(int argc, char *argv[])
{
    if(argc < 3) {
        fprintf(2, "Usage: find...\n");
        exit(1);
    }

    char *path = argv[1];
    char *name = argv[2];

    find(path, name);
    
    exit(0);
}