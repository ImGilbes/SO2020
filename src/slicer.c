#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "list.h"
#include "itoa.h"
#include "file_analysis.h"

#include "settings.h"

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

struct list *files_analysis;

// FIX la path per il reader secondo le specifiche del professore

// aggiorna (incrementa) le occorrenze di `occurrences` del carattere `char_int`
// per il file `file`
void update_file_analysis(char *file, int char_int, int occurrences)
{
    struct list_iterator *files_analysis_iter = list_iterator_new(files_analysis);
    struct file_analysis *files_analysis;

    while ((files_analysis = (struct file_analysis *)list_iterator_next(files_analysis_iter)))
    {
        if (strcmp(files_analysis->file, file) == 0)
        {
            // la concorrenza potrebbe creare problemi di consistenza nella tabella
            // accesso mutuamente esclusivo alla sezione critica
            pthread_mutex_lock(&mtx);
            files_analysis->analysis[char_int] += occurrences;
            pthread_mutex_unlock(&mtx);
            break;
        }
    }

    list_iterator_delete(files_analysis_iter);
}

// funzione eseguita da thread (un thread per ogni slice)
// legge l'output di un processo reader sino alla sua conclusione,
// estrapola informazioni e aggiorna le occorrenze indicate
void reader_listener(void *fd_v)
{
    int fd = *((int *)fd_v);
    char a_char[2] = {'\0', '\0'}; // cosi' possiamo usare strcat
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

            if (file_analysis_parse_line(a_line, &file, &char_int, &occurrences))
            {
                // aggiornamento occorrenze
                update_file_analysis(file, char_int, occurrences);
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

// struttura per contenere dati che correlano processi reader, thread listener e le pipe che li mettono in comunicazione
struct communication
{
    pid_t reader_id;
    pthread_t listener_id;
    int pipe[2];
};

int main(int argc, char **argv, char **env)
{
    // controllo esistenza degli eseguibili chiamati da execve
    if (is_executable("bin/reader") != 1)
    {
        fprintf(stderr, "Non e' possibile trovare l'eseguibile bin/reader\n");
        return;
    }

    int number_of_slices = 4;
    files_analysis = list_new();

    // parsing dei parametri della chiamata
    int arg_index = 1; // l'elemento alla posizione 0 e' la stringa di chiamata del programma
    while (arg_index < argc)
    {
        if (strcmp(argv[arg_index], "-m") == 0)
        {
            number_of_slices = atoi(argv[++arg_index]);
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

    // creazione strutture dati per agglomerare informazioni per comunicazione tra thread e processi
    struct communication *comms = (struct communication *)malloc(sizeof(struct communication) * number_of_slices);

    // creazione dei canali di comunicazione, dei processi reader e dei thread listener
    int slice_id;
    for (slice_id = 1; slice_id <= number_of_slices; slice_id++)
    {
        pipe(comms[slice_id - 1].pipe);

        bool waiting = false;
        while ( (comms[slice_id - 1].reader_id = fork()) == -1 )
        {
            if (waiting == false)
            {
                fprintf(stderr, "in attesa di risorse\n");
                waiting = true;
            }

            usleep(100);
        }

        if (comms[slice_id - 1].reader_id == 0)
        {
            // sostituzione immagine processo con reader

            // creazione argomenti per la chiamata a reader
            char **reader_argv = (char **)malloc(sizeof(char *) * (files_analysis->lenght + 6)); // 6: ./reader, -s, slice_id, -m, number_of_slices, NULL
            int arg_index = 0;

            // primo parametro: nome con cui e' invocato l'eseguibile. non e' importante sia corretto
            reader_argv[arg_index++] = "./reader";

            // impostazione parametri per identificativo dello slice
            reader_argv[arg_index++] = "-s";
            char slice_id_arg[8];
            itoa(slice_id, slice_id_arg);
            reader_argv[arg_index++] = slice_id_arg;

            // impostazione parametri per quantita' di slice
            reader_argv[arg_index++] = "-m";
            char number_of_slices_arg[8];
            itoa(number_of_slices, number_of_slices_arg);
            reader_argv[arg_index++] = number_of_slices_arg;

            // impostazione parametri per i file da analizzare
            struct list_iterator *files_analysis_iter = list_iterator_new(files_analysis);
            struct file_analysis *file_analysis;
            while ((file_analysis = (struct file_analysis *)list_iterator_next(files_analysis_iter)))
            {
                reader_argv[arg_index++] = file_analysis->file;
            }

            // null-terminated vector
            reader_argv[arg_index] = NULL;

            // redirezione dell'output nella write-end della pipe
            dup2(comms[slice_id - 1].pipe[1], STDOUT_FILENO);
            close(comms[slice_id - 1].pipe[0]);
            close(comms[slice_id - 1].pipe[1]);

            execve("bin/reader", reader_argv, env);
        }

        else
        {
            // avvio thread per eseguire il listener
            close(comms[slice_id - 1].pipe[1]);

            bool waiting = false;
            while (pthread_create(&comms[slice_id - 1].listener_id, NULL, (void *)reader_listener, (void *)&comms[slice_id - 1].pipe[0]))
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
    while (threads_done < number_of_slices)
    {
        pthread_join(comms[threads_done++].listener_id, NULL);
    }

    free(comms);

    // stampa della tabella di analisi
    struct list_iterator *files_analysis_iter = list_iterator_new(files_analysis);
    struct file_analysis *file_analysis;

    while ((file_analysis = (struct file_analysis *)list_iterator_next(files_analysis_iter)))
    {
        int char_int = 0;
        while (char_int < 128)
        {
            if (file_analysis->analysis[char_int] > 0)
            {
                printf("%s:%d:%lu\n", file_analysis->file, char_int, file_analysis->analysis[char_int]);
            }
            char_int++;
        }

        file_analysis_delete(file_analysis);
    }

    list_iterator_delete(files_analysis_iter);
    list_delete(files_analysis);
}