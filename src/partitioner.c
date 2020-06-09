#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#include "list.h"
#include "file_analysis.h"
#include "itoa.h"
#include "fs.h"

#include "settings.h"

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

struct
{
    int number_of_partitions;
    int number_of_slices;
} partition_properties = {3, 4};

// struttura per contenere dati che correlano processi slicer, thread listener e le pipe che li mettono in comunicazione
struct communication
{
    pid_t slicer_id;
    pthread_t listener_id;
    int pipe[2];
};

// funzione eseguita da thread (un thread per ogni slice)
// legge l'output di un processo reader sino alla sua conclusione,
// e lo riporta nel suo output (non deve fare computazioni)
void slicer_listener(void *fd_v)
{
    int fd = *((int *)fd_v);
    char a_char[2] = {'\0', '\0'};
    char a_line[LINE_SIZE] = {'\0'};
    int bytes;

    while ((bytes = read(fd, a_char, 1)) > 0)
    {
        if (a_char[0] == '\n')
        {
            // linea completata, pronta per essere analizzata
            char *file;
            int char_int;
            unsigned long occurrences;

            // non mi interessano i dati, ma controllo ugualmente siano nel formato corretto
            if (file_analysis_parse_line(a_line, &file, &char_int, &occurrences))
            {
                // la concorrenza potrebbe creare problemi di sovrapposizione di output
                // accesso mutuamente esclusivo alla sezione critica
                pthread_mutex_lock(&mtx);
                printf("%s:%d:%lu\n", file, char_int, occurrences);
                pthread_mutex_unlock(&mtx);

                free(file);
            }

            // resetta la linea
            a_line[0] = '\0';
        }
        else
        {
            // carattere di una linea, concatenazione
            strcat(a_line, a_char);
        }
    }

    close(fd);
}

int main(int argc, char **argv, char **env)
{
    // controllo esistenza degli eseguibili chiamati da execve
    if (is_executable("bin/slicer") != 1)
    {
        fprintf(stderr, "Non e' possibile trovare l'eseguibile bin/slicer\n");
        return;
    }

    struct list *files_analysis = list_new();

    // parsing dei parametri della chiamata
    int arg_index = 1; // l'elemento alla posizione 0 e' la stringa di chiamata del programma
    while (arg_index < argc)
    {
        if (strcmp(argv[arg_index], "-n") == 0)
        {
            partition_properties.number_of_partitions = atoi(argv[++arg_index]);
        }
        else if (strcmp(argv[arg_index], "-m") == 0)
        {
            partition_properties.number_of_slices = atoi(argv[++arg_index]);
        }
        else
        {
            char *file = (char *)malloc(sizeof(char) * (strlen(argv[arg_index]) + 1));
            strcpy(file, argv[arg_index]);

            struct file_analysis *file_analysis = file_analysis_new();
            file_analysis->file = file;
            list_push(files_analysis, file_analysis);
        }

        arg_index++;
    }

    // creazione di partizionamenti
    struct list **files_in_partition = (struct list **)malloc(sizeof(struct list *) * partition_properties.number_of_partitions);
    struct list_iterator *files_analysis_iter;
    struct file_analysis *file_analysis;

    // numero massimo di file in una partizione
    int files_per_partition = ceil(((double)files_analysis->lenght) / partition_properties.number_of_partitions);

    files_analysis_iter = list_iterator_new(files_analysis);

    // assegnazione dei file nelle partizioni
    int partition_id;
    for (partition_id = 1; partition_id <= partition_properties.number_of_partitions; partition_id++)
    {
        files_in_partition[partition_id - 1] = list_new();

        while (files_in_partition[partition_id - 1]->lenght < files_per_partition && (file_analysis = (struct file_analysis *)list_iterator_next(files_analysis_iter)))
        {
            list_push(files_in_partition[partition_id - 1], file_analysis);
        }
    }

    list_iterator_delete(files_analysis_iter);

    // creazione strutture dati per agglomerare informazioni per comunicazione tra thread e processi
    struct communication *comms = (struct communication *)malloc(sizeof(struct communication) * partition_properties.number_of_partitions);

    // creazione dei canali di comunicazione, dei processi reader e dei thread listener
    for (partition_id = 1; partition_id <= partition_properties.number_of_partitions; partition_id++)
    {
        pipe(comms[partition_id - 1].pipe);

        bool waiting = false;
        while ( (comms[partition_id - 1].slicer_id = fork()) == -1 )
        {
            if (waiting == false)
            {
                fprintf(stderr, "in attesa di risorse\n");
                waiting = true;
            }

            usleep(100);
        }

        if (comms[partition_id - 1].slicer_id == 0)
        {
            // sostituzione immagine processo con reader

            // creazione argomenti per la chiamata a reader
            char **slicer_argv = (char **)malloc(sizeof(char *) * (files_per_partition + 4)); // 4: ./slicer, -m, number_of_slices, NULL
            int arg_index = 0;

            // primo parametro: nome con cui e' invocato l'eseguibile. non e' importante sia corretto
            slicer_argv[arg_index++] = "./slicer";

            // impostazione parametri per quantita' di split
            slicer_argv[arg_index++] = "-m";
            char number_of_slices_arg[8];
            itoa(partition_properties.number_of_slices, number_of_slices_arg);
            slicer_argv[arg_index++] = number_of_slices_arg;

            // impostazione parametri per i file da analizzare
            files_analysis_iter = list_iterator_new(files_in_partition[partition_id - 1]);
            while ((file_analysis = (struct file_analysis *)list_iterator_next(files_analysis_iter)))
            {
                slicer_argv[arg_index++] = file_analysis->file;
            }
            // null-terminated vector
            slicer_argv[arg_index] = NULL;

            // redirezione dell'output nella write-end della pipe
            dup2(comms[partition_id - 1].pipe[1], STDOUT_FILENO);
            close(comms[partition_id - 1].pipe[0]);
            close(comms[partition_id - 1].pipe[1]);

            execve("bin/slicer", slicer_argv, env);
        }

        else
        {
            // avvio thread per eseguire il listener
            close(comms[partition_id - 1].pipe[1]);

            bool waiting = false;
            while ( pthread_create(&comms[partition_id - 1].listener_id, NULL, (void *)slicer_listener, (void *)&comms[partition_id - 1].pipe[0]) )
            {
                if (waiting == false)
                {
                    fprintf(stderr, "in attesa di risorse\n");
                    waiting = true;
                }

                usleep(100);
            }
        }
    }

    // attesa della conclusione di tutti i thread
    int threads_done = 0;
    while (threads_done < partition_properties.number_of_partitions)
    {
        pthread_join(comms[threads_done++].listener_id, NULL);
    }

    list_delete(files_analysis);
}
