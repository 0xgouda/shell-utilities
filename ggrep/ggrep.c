#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    if (argc == 1) {
        printf("ggrep: searchterm [file ...]\n");
        return 1;
    }

    char* matchstring = argv[1];
    int i = 2;
    do {
        FILE* file;
        if (argc > 2) { 
            file = fopen(argv[i], "r");
            if (file == NULL) {
                printf("ggrep: cannot open file %s\n", argv[i]);
                return 1;
            }
        } else {
            file = stdin; 
        }

        char* buffer = NULL; size_t bufferlen = 0;
        while(getline(&buffer, &bufferlen, file) != -1) {
            if (strstr(buffer, matchstring) != NULL) {
                printf("%s", buffer);
            }
        }
        free(buffer); ++i;
        fclose(file);
    } while(i < argc);
}