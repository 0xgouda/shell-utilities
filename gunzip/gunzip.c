#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        printf("wunzip: file1 [file2 ...]\n");
        return 1;
    }

    for (int i = 1; i < argc; i++)
    {
        FILE *file = fopen(argv[i], "rb");

        char buffer;
        int number;
        while (fread(&number, sizeof(int), 1, file) && 
                fread(&buffer, sizeof(char), 1, file))
        {

            for (int i = 0; i < number; i++)
            {
                printf("%c", buffer);
            }
        }
        fclose(file);
    }
}