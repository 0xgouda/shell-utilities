#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    if (argc == 1) {
        printf("wzip: file1 [file2 ...]\n");
        return 1;
    }

    int counter = 1; char current_char = '\0';
    int first_char = 1;
    for (int j = 1; j < argc; j++) {
        FILE* file = fopen(argv[j], "rb");

        char buffer[4096]; size_t bufferlen;
        while((bufferlen = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            for (int i = 0; i < bufferlen; i++) {
                if (first_char) {
                    current_char = buffer[0];
                    counter = 1; first_char = 0;
                } else if (buffer[i] == current_char) {
                    counter++;
                } else {
                    fwrite(&counter, sizeof(int), 1, stdout);
                    fwrite(&current_char, sizeof(char), 1, stdout);
                    counter = 1; current_char = buffer[i];          
                }
            }
        }
        fclose(file);
    }
    fwrite(&counter, sizeof(int), 1, stdout);
    fwrite(&current_char, sizeof(char), 1, stdout);
}