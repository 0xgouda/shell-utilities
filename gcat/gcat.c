#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc == 1) return 0;

    for (int i = 1; i < argc; i++) {
        FILE* path = fopen(argv[i], "r");
        if (path == NULL) {
            printf("gcat: cannot open file %s\n", argv[i]);
            return 1;
        }

        char buffer[2048];
        while(fgets(buffer, sizeof(buffer), path) != NULL) {
            printf("%s", buffer);
        }
        fclose(path);
    }  
}