#include <stdio.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <pathname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i++) {
        struct stat s;

        if (lstat(argv[i], &s) == -1) {
           perror("Error retriving file state");
           exit(EXIT_FAILURE); 
        }

        printf("File: %s\n", argv[i]);
        printf("Size: %ld\tBlocks: %ld\tIO Block: %ld\t", s.st_size, s.st_blocks, s.st_blksize);

        switch(s.st_mode & __S_IFMT) {
            case __S_IFBLK: printf("block device\n"); break;
            case __S_IFCHR: printf("character device\n"); break;
            case __S_IFDIR: printf("directory\n"); break;
            case __S_IFIFO: printf("pipe\n"); break;
            case __S_IFLNK: printf("symlink\n"); break;
            case __S_IFREG: printf("regular file\n"); break;
            case __S_IFSOCK: printf("socket\n"); break;
            default: printf("unknown\n");
        }

        printf("Device: %d,%d\tInode: %ld\tLinks: %ld\n", major(s.st_dev), minor(s.st_dev), s.st_ino, s.st_nlink);
        printf("Access: UID=%d\tGID=%d\n", s.st_uid, s.st_gid);
        printf("Access: %s", ctime(&s.st_atime));
        printf("Modify: %s", ctime(&s.st_mtime));
        printf("Change: %s", ctime(&s.st_ctime));
    }
    exit(EXIT_SUCCESS);
}