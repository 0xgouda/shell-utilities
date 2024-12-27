#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct {
    int num;
    char letter;
} element;

// TODO: many global vars, return or exit(1), concatenation
int curr_file = 1;
int numOfFiles;
char ** args;
pthread_mutex_t curr_file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t write_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t write_wait = PTHREAD_COND_INITIALIZER;
char *ptr = NULL;
struct stat filestat;
size_t curr_chunk;
int WORK_SIZE = 1048576;
int numOfThreads;
int current_writer = 0;

void *worker(void *my_thread_id) {
    element *my_buffer = (element *) malloc(WORK_SIZE * sizeof(element));
    if (my_buffer == NULL) {
        printf("Error Allocating Memory for thread %d", *(int *)my_thread_id);
        exit(1);
    }

    while (1) {
        int my_chunk = -1, my_limit = -1, writer_id;
        char *local_ptr = NULL;
        size_t sz;

        pthread_mutex_lock(&curr_file_lock);
        if (numOfFiles <= 0) {
            pthread_mutex_unlock(&curr_file_lock);
            free(my_buffer);
            return NULL;
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
                printf("Error retrieving state of file: %s", args[curr_file]);
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
        writer_id = current_writer;
        pthread_mutex_unlock(&curr_file_lock);


        int buffer_len = 0;

        int counter = 1; char current_char = '\0';
        int first_char = 1;
        for (int i = my_chunk; i < (my_chunk + my_limit); i++) {
            if (first_char) {
                current_char = local_ptr[i];
                counter = 1; first_char = 0;
            } else if (local_ptr[i] == current_char) {
                counter++;
            } else {
                my_buffer[buffer_len].num = counter;
                my_buffer[buffer_len].letter = current_char;
                buffer_len++;
                counter = 1; current_char = local_ptr[i];
            }
        }

        pthread_mutex_lock(&write_lock);
        while (current_writer != writer_id) {
            pthread_cond_wait(&write_wait, &write_lock);
        }

        fwrite(my_buffer, sizeof(element), buffer_len, stdout);
        current_writer = (current_writer + 1) % numOfThreads;

        pthread_cond_signal(&write_wait);
        pthread_mutex_unlock(&write_lock);

        if (my_chunk + my_limit >= sz) {
            munmap(local_ptr, sz);
        }
    }
}

int main (int argc, char** argv) {
    if (argc == 1) {
        printf("gpzip: file1 [file2 ...]\n");
        return 1;
    }

    numOfFiles = argc - 1;
    args = argv;

    numOfThreads = get_nprocs();
    pthread_t threads[numOfThreads];
    int *tmp = malloc(sizeof(int) * numOfThreads);
    for (int i = 0; i < numOfThreads; i++) {
        tmp[i] = i;
        pthread_create(&threads[i], NULL, worker, &tmp[i]);
    }

    for (int i = 0; i < numOfThreads; i++) {
        pthread_join(threads[i], NULL);
    }
    free(tmp);
}