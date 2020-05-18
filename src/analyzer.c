#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "list.h"
#include "fs.h"

int main(int argc, char **argv, char **env) {
    struct list* files = list_new();
    struct list* dirs = list_new();
    int number_of_partitions;
    int number_of_slices;

    // parsing dei parametri della chiamata
    int arg_index = 1;
    while (arg_index < argc)
    {
        if (strcmp(argv[arg_index], "-n") == 0)
        {
            number_of_partitions = atoi(argv[++arg_index]);
        }
        else if (strcmp(argv[arg_index], "-m") == 0)
        {
            number_of_slices = atoi(argv[++arg_index]);
        }
        else
        {
            char *file = (char *)malloc(sizeof(char) * strlen(argv[arg_index]));
            strcpy(file, argv[arg_index]);

            if (is_directory(file) == -1) {
                printf("%s non esiste. ignorato.\n", file);
            } else {
                if (is_directory(file)) {
                    list_push(dirs, file);
                    printf("%s e' una directory\n", file);
                } else {
                    list_push(files, file);
                    printf("%s e' un file\n", file);
                }
            }
        }
        arg_index++;
    }

    // aggiunta in profondita' del contenuto delle directory
    struct list_iterator *dirs_iter = list_iterator_new(dirs);
    char *dir;
    
    while ((dir = (char*)list_iterator_next(dirs_iter))) {
        struct list *files_in_dir = ls(dir);
        struct list_iterator *files_in_dir_iter = list_iterator_new(files_in_dir);
        char *file;

        while ((file = (char*)list_iterator_next(files_in_dir_iter))) {
            if (is_directory(file) == -1) {
                printf("'%s' non esiste. ignorato.\n", file);
            } else {
                if (is_directory(file)) {
                    list_push(dirs, file);
                    printf("%s e' una directory\n", file);
                } else {
                    list_push(files, file);
                    printf("%s e' un file\n", file);
                }
            }
        }
    }

    // // redirezione output ad una fifo (per report)?
    // dup2(fifo o altro, STDOUT_FILENO);

    // // execve con il vettore contenente i file
    // da lista a vettore nul terminato
    // execve(...);
}