#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "itoa.h"
#include "list.h"
#include "fs.h"
#include "utilities.h"
#include "bool.h"

int main(int argc, char **argv, char **env)
{
    struct list *files = list_new();
    struct list *dirs = list_new();
    const int MAX_NUMBER_OF_DIGETS = 10;
    int number_of_partitions = 3;
    int number_of_slices = 4;
    char *path_of_fifo = NULL;
    char *partitioner_id;
    bool is_asked_for_stdout = true;
    bool is_asked_for_report = false;

    // Parsing dei parametri della chiamata

    int arg_index = 1;

    while (arg_index < argc)
    {
        if (strcmp(argv[arg_index], "-r") == 0)
        {
            arg_index++;
            partitioner_id = argv[arg_index];

            if (partitioner_id != NULL)
            {
                is_asked_for_report = true;
                path_of_fifo = (char *)malloc(sizeof(char) * (5 + strlen(partitioner_id) + 1));
                strcpy(path_of_fifo, "/tmp/");
                strcat(path_of_fifo, partitioner_id);
            }
            else
            {
                fprintf(stderr, "Il parametro r non e' passato\n");
            }
        }
        else if (strcmp(argv[arg_index], "-n") == 0)
        {
            arg_index++;

            if (argv[arg_index] != NULL)
            {
                if (is_positive_number(argv[arg_index]))
                {
                    number_of_partitions = atoi(argv[arg_index]);
                }
                else
                {
                    fprintf(stderr, "Il parametro n deve essere un intero positivo\nValore di default: %d\n", number_of_partitions);
                }
            }
            else
            {
                fprintf(stderr, "Il parametro n non e' passato\n");
            }
        }
        else if (strcmp(argv[arg_index], "-m") == 0)
        {
            arg_index++;

            if (argv[arg_index] != NULL)
            {
                if (is_positive_number(argv[arg_index]))
                {
                    number_of_slices = atoi(argv[arg_index]);
                }
                else
                {
                    fprintf(stderr, "Il parametro m deve essere un intero positivo\nValore di default: %d\n", number_of_slices);
                }
            }
            else
            {
                fprintf(stderr, "Il parametro m non e' passato\n");
            }
        }
        else
        {
            char *file = (char *)malloc(sizeof(char) * (strlen(argv[arg_index]) + 1));
            strcpy(file, argv[arg_index]);

            if (is_directory(file) == -1)
            {
                fprintf(stderr, "%s non esiste. ignorato.\n", file);
            }
            else
            {
                if (is_directory(file))
                {
                    list_push(dirs, file);
                }
                else
                {
                    list_push(files, file);
                }
            }
        }
        arg_index++;
    }

    // Aggiunta in profondita' del contenuto delle directory

    while (!list_is_empty(dirs))
    {
        char *dir = (char *)list_pop(dirs);
        struct list *files_in_dir = ls(dir);
        struct list_iterator *files_in_dir_iter = list_iterator_new(files_in_dir);
        char *file;

        while ((file = (char *)list_iterator_next(files_in_dir_iter)))
        {
            if (is_directory(file) == -1)
            {
                fprintf(stderr, "%s non esiste. ignorato.\n", file);
            }
            else
            {
                if (is_directory(file))
                {
                    list_push(dirs, file);
                }
                else
                {
                    list_push(files, file);
                }
            }
        }

        list_iterator_delete(files_in_dir_iter);
    }

    list_delete(dirs);

    // Preparazione dei paramentri del nuovo processo partitioner da passare alla execve

    int index_of_arg = 0;
    int number_of_files = files->lenght;
    char **partitioner_argv = (char **)malloc(sizeof(char *) * (number_of_files + 6)); // ./partitioner -n 3 -m 4 number_of_files NULL

    partitioner_argv[index_of_arg++] = "bin/partitioner";

    partitioner_argv[index_of_arg++] = "-n";

    char number_of_partitions_arg[MAX_NUMBER_OF_DIGETS];
    itoa(number_of_partitions, number_of_partitions_arg);
    partitioner_argv[index_of_arg++] = number_of_partitions_arg;

    partitioner_argv[index_of_arg++] = "-m";

    char number_of_slices_arg[MAX_NUMBER_OF_DIGETS];
    itoa(number_of_slices, number_of_slices_arg);
    partitioner_argv[index_of_arg++] = number_of_slices_arg;

    struct list_iterator *iterator = list_iterator_new(files);
    char *file;

    while ((file = (char *)list_iterator_next(iterator)) != NULL)
    {
        partitioner_argv[index_of_arg++] = file;
    }

    list_iterator_delete(iterator);

    partitioner_argv[index_of_arg] = NULL;

    // Apertura della fifo, redirezione dello stdout sulla fifo in modalita' di sola scrittura
    // con i controlli sul successo di tali operazioni.
    // Questo solo quando si chiama l'analyzer nel seguente modo: ./analyzer -r 489 -n 3 -m 4 file1.txt file2.txt ...

    if (is_asked_for_report)
    {
        is_asked_for_stdout = false;

        int fd = open(path_of_fifo, O_WRONLY);

        if (fd == -1)
        {
            fprintf(stderr, "Errore apertura fifo: %s\n", path_of_fifo);
            is_asked_for_report = false;
        }
        else
        {
            int fd_new = dup2(fd, STDOUT_FILENO);
            close(fd);

            if (fd_new == -1)
            {
                fprintf(stderr, "Errore duplicazine file descriptor\n");
                is_asked_for_report = false;
            }
        }
    }

    // Esecuzione del comando execve() con il relativo controllo sul successo dell'operazione.

    if (is_asked_for_report || is_asked_for_stdout)
    {
        int result = execve(partitioner_argv[0], partitioner_argv, env);

        if (result == -1)
        {
            fprintf(stderr, "Errore esecuzione execve\n");
        }
    }

    return 0;
}