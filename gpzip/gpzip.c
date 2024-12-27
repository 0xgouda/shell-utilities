#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdlib.h>

// TODO: many global vars
int curr_file = 1;
int numOfFiles;
char ** args;
pthread_mutex_t curr_file_lock = PTHREAD_MUTEX_INITIALIZER;
char *ptr = NULL;
struct stat filestat;
size_t curr_chunk;
int WORK_SIZE = 1048576;

void worker() {
    while (1) {
        int my_chunk = -1, my_limit = -1;
        char *local_ptr = NULL;
        size_t sz;

        pthread_mutex_lock(&curr_file_lock);
        if (numOfFiles <= 0) {
            pthread_mutex_unlock(&curr_file_lock);
            return;
        }

        if (ptr == NULL) {
            curr_chunk = 0;
            FILE *file = fopen(args[curr_file], "rb");
            if (file == NULL) {
                printf("Failed to open file: %s", args[curr_file]);
                exit(1);
            }
            int fd = fileno(file);


            if (fstat(fd, &filestat) == -1) {
                printf("Error retriving state of file: %s", args[curr_file]);
                exit(1);
            }

            ptr = (char *) mmap(NULL, filestat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, curr_chunk);
            if (ptr == MAP_FAILED) {
                ptr = NULL;
                pthread_mutex_unlock(&curr_file_lock);
                continue;
            }

            numOfFiles--;
            curr_file++;
        }

        if (curr_chunk < filestat.st_size) {
            my_chunk = curr_chunk;
            if (WORK_SIZE < filestat.st_size - curr_chunk) {
                my_limit = WORK_SIZE;
            } else {
                my_limit = filestat.st_size - curr_chunk;
            }
            curr_chunk += WORK_SIZE;
        } else {
            ptr = NULL;
            pthread_mutex_unlock(&curr_file_lock);
            continue;
        }

        local_ptr = ptr; sz = filestat.st_size;
        pthread_mutex_unlock(&curr_file_lock);


        int counter = 1; char current_char = '\0';
        int first_char = 1;
        for (int i = my_chunk; i < (my_chunk + my_limit); i++) {
            if (first_char) {
                current_char = local_ptr[i];
                counter = 1; first_char = 0;
            } else if (local_ptr[i] == current_char) {
                counter++;
            } else {
                fwrite(&counter, sizeof(int), 1, stdout);
                fwrite(&current_char, sizeof(char), 1, stdout);
                counter = 1; current_char = local_ptr[i];
            }
        }

        if (my_chunk + my_limit >= sz) {
            munmap(local_ptr, sz);
        }
    }
}

int main (int argc, char** argv) {
    if (argc == 1) {
        printf("gzip: file1 [file2 ...]\n");
        return 1;
    }

    numOfFiles = argc - 1;
    args = argv;

    int numOfCpus = get_nprocs();
    pthread_t threads[numOfCpus];
    for (int i = 0; i < numOfCpus; i++) {
        pthread_create(&threads[i], NULL, (void *) worker, NULL);
    }

    for (int i = 0; i < numOfCpus; i++) {
        pthread_join(threads[i], NULL);
    }
}