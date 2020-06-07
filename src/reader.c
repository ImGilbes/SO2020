#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#include "list.h"
#include "file_analysis.h"
#include "fs.h"
#include "utilities.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

struct
{
    int slice_id;
    int number_of_slices;
} slice_properties = {1, 1}; // lettura completa del file di default

void read_slice(void *file_analysis_vp);

int main(int argc, char **argv, char **env)
{
    struct list *files_analysis = list_new();

    // parsing dei parametri della chiamata
    int arg_index = 1; // l'elemento alla posizione 0 e' la stringa di chiamata del programma
    while (arg_index < argc)
    {
        if (strcmp(argv[arg_index], "-s") == 0)
        {
            slice_properties.slice_id = atoi(argv[++arg_index]); // FIXME solo cifre
        }
        else if (strcmp(argv[arg_index], "-m") == 0)
        {
            slice_properties.number_of_slices = atoi(argv[++arg_index]); // FIXME solo cifre
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

    // avvio dei thread per la lettura delle slice dei file
    struct list_iterator *files_analysis_iter = list_iterator_new(files_analysis);
    struct file_analysis *file_analysis;
    pthread_t *thread_ids = malloc(sizeof(pthread_t) * files_analysis->lenght);

    int thread_index = 0;
    while ((file_analysis = (struct file_analysis *)list_iterator_next(files_analysis_iter)))
    {
        pthread_create(&thread_ids[thread_index++], NULL, (void *)read_slice, (void *)file_analysis);
    }

    // attesa della conclusione di tutti i thread
    thread_index = 0;
    while (thread_index < files_analysis->lenght)
    {
        pthread_join(thread_ids[thread_index++], NULL);
    }

    // stampa della lista delle occorrenze in stdout
    list_iterator_delete(files_analysis_iter);
    files_analysis_iter = list_iterator_new(files_analysis);

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

// legge la slice #`slice_properties.slice_id` delle `slice_properties.number_of_slices` slice
// che compongono il file `file_analysis->file`. salva le analisi in `file_analysis->analysis`
void read_slice(void *file_analysis_vp)
{
    struct file_analysis *file_analysis = (struct file_analysis *)file_analysis_vp;

    if (is_ascii_file(file_analysis->file) != 1) {
        // non e' un file ascii, ignorarlo
        return;
    }

    FILE *file = fopen(file_analysis->file, "r");
    if (file != NULL)
    {
        // ottenimento dimensione del file
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);

        // calcolo estremi dello slice da considerare
        long slice_size = ceil((double)file_size / slice_properties.number_of_slices);
        long slice_begin = (slice_properties.slice_id - 1) * slice_size;
        long slice_end = MIN(file_size - 1, slice_begin + slice_size);

        // spostamento inizio dello slice
        fseek(file, slice_begin, 0);

        // lettura caratteri e conteggio
        int char_read;
        while ((char_read = fgetc(file)) != EOF && slice_size > 0)
        {
            file_analysis->analysis[(char)char_read]++;
            slice_size--;
        }

        fclose(file);
    }
}