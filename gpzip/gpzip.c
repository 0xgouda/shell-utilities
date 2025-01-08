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

char *ptr = NULL;
struct stat filestat;
size_t curr_chunk;
int THREAD_CHUNK = 1048576;
int NUM_OF_THREAD_BUFFER_ELEMENTS = 100000;
int current_writer = 0;
int writeId = 0;
element last_element = {0, '\0'};

void write_buffer(int writer_id, element *my_buffer, int buffer_len) {
    pthread_mutex_lock(&write_lock);

    while (current_writer != writer_id) {
        pthread_cond_wait(&write_wait, &write_lock);
    }

    if (buffer_len && my_buffer[0].letter == last_element.letter) {
        my_buffer[0].num += last_element.num;
    } else {
        fwrite(&last_element.num, sizeof(int), 1, stdout);
        putchar(last_element.letter);
    }

    for (int i = 0; i < buffer_len - 1; i++) {
        fwrite(&my_buffer[i].num, sizeof(int), 1, stdout);
        putchar(my_buffer[i].letter);
    }
    last_element = my_buffer[buffer_len - 1];

    pthread_mutex_unlock(&write_lock);
}

void done_writing() {
    pthread_mutex_lock(&write_lock);

    current_writer++;
    pthread_cond_broadcast(&write_wait);

    pthread_mutex_unlock(&write_lock);
}

int open_new_file(element *my_buffer) {
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

    ptr = (char *) mmap(NULL, filestat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (ptr == MAP_FAILED) {
        fprintf(stderr, "Failed to Allocate Memory\n");
        exit(1);
    }

    numOfFiles--;
    curr_file++;

    return 1;
}

void *worker(void *my_thread_id) {
    element *my_buffer = (element *) malloc(NUM_OF_THREAD_BUFFER_ELEMENTS * sizeof(element));
    if (my_buffer == NULL) {
        fprintf(stderr, "Error Allocating Memory for thread %d\n", *(int *)my_thread_id);
        exit(1);
    }

    while (1) {
        pthread_mutex_lock(&curr_file_lock);

        if (open_new_file(my_buffer) == 0) {
            return NULL;
        }

        int my_chunk = curr_chunk;
        char* local_ptr = ptr;
        curr_chunk += THREAD_CHUNK;
        size_t sz = filestat.st_size;

        int my_limit = 0;
        if (curr_chunk <= filestat.st_size) {
            my_limit = THREAD_CHUNK;
        } else {
            my_limit = filestat.st_size - (curr_chunk - THREAD_CHUNK);
            ptr = NULL;
        }

        int writer_id = writeId;
        writeId++;

        pthread_mutex_unlock(&curr_file_lock);


        int buffer_len = 0, counter = 1, first_char = 1;
        char current_char = '\0';
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
                if (buffer_len > NUM_OF_THREAD_BUFFER_ELEMENTS) {
                    write_buffer(writer_id, my_buffer, buffer_len);
                    buffer_len = 0;
                }
                counter = 1; current_char = local_ptr[i];
            }
        }
        my_buffer[buffer_len].num = counter;
        my_buffer[buffer_len].letter = current_char;
        buffer_len++;

        write_buffer(writer_id, my_buffer, buffer_len);
        done_writing();

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
    fwrite(&last_element.num, sizeof(int), 1, stdout);
    putchar(last_element.letter);

    free(tmp);
}