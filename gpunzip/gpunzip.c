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

int curr_file = 1;
int numOfFiles;
char ** args;
int numOfThreads;

pthread_mutex_t curr_file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t write_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t write_wait = PTHREAD_COND_INITIALIZER;

element *ptr = NULL;
struct stat filestat;
size_t curr_chunk;
int THREAD_BUFFER_SIZE = 1048576;
int NUM_OF_THREAD_CHUNK_ELEMENTS = 3072;
int current_writer = 0;
int writeId = 0;

void write_buffer(int writer_id, char *my_buffer, int buffer_len) {
    pthread_mutex_lock(&write_lock);

    while (current_writer != writer_id) {
        pthread_cond_wait(&write_wait, &write_lock);
    }
    fwrite(my_buffer, sizeof(char), buffer_len, stdout);

    pthread_mutex_unlock(&write_lock);
}

void done_writing() {
    pthread_mutex_lock(&write_lock);

    current_writer++;
    pthread_cond_broadcast(&write_wait);
    
    pthread_mutex_unlock(&write_lock);
}

int open_new_file(char *my_buffer) {
    if (ptr != NULL) return 1;

    if (numOfFiles <= 0) {
        pthread_mutex_unlock(&curr_file_lock);
        free(my_buffer);
        return 0;
    }

    curr_chunk = 0;
    FILE *file = fopen(args[curr_file], "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file: %s\n", args[curr_file]);
        exit(1);
    }
    int fd = fileno(file);

    if (fstat(fd, &filestat) == -1) {
        fprintf(stderr, "Error retrieving state of file: %s\n", args[curr_file]);
        exit(1);
    }

    ptr = (element *) mmap(NULL, filestat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (ptr == MAP_FAILED) {
        fprintf(stderr, "Failed to Allocate Memory\n");
        exit(1);
    }

    numOfFiles--;
    curr_file++;

    return 1;
}

void *worker(void *my_thread_id) {
    char *my_buffer = (char *) malloc(THREAD_BUFFER_SIZE);
    if (my_buffer == NULL) {
        fprintf(stderr, "Error Allocating Memory for thread %d\n", *(int *)my_thread_id);
        return NULL;
    }

    while (1) {
        pthread_mutex_lock(&curr_file_lock);

        if (open_new_file(my_buffer) == 0) {
            return NULL;
        }

        int my_chunk = curr_chunk;
        element *local_ptr = ptr + my_chunk;
        curr_chunk += NUM_OF_THREAD_CHUNK_ELEMENTS;
        size_t sz = filestat.st_size;

        int my_limit = 0;
        if (curr_chunk * sizeof(element) <= filestat.st_size) {
            my_limit = NUM_OF_THREAD_CHUNK_ELEMENTS;
        } else {
            my_limit = (filestat.st_size / sizeof(element)) % NUM_OF_THREAD_CHUNK_ELEMENTS;
            ptr = NULL;
        }

        int writer_id = writeId;
        writeId++;

        pthread_mutex_unlock(&curr_file_lock);

        int buffer_len = 0;
        for (int i = 0; i < my_limit; local_ptr++, i++) {
            for (int j = 0; j < local_ptr->num; j++) {
                my_buffer[buffer_len] = local_ptr->letter;
                buffer_len++;

                if (buffer_len >= THREAD_BUFFER_SIZE) {
                    write_buffer(writer_id, my_buffer, buffer_len);
                    buffer_len = 0;
                }
            }
        }
        write_buffer(writer_id, my_buffer, buffer_len);
        done_writing();

        if ((my_chunk + my_limit) * sizeof(element) >= sz) {
            munmap(local_ptr - my_chunk, sz);
        }
    }
}

int main (int argc, char** argv) {
    if (argc == 1) {
        printf("gpunzip: file1 [file2 ...]\n");
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