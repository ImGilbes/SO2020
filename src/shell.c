#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#include "bool.h"
#include "list.h"
#include "file_analysis.h"
#include "itoa.h"
#include "utilities.h"
#include "settings.h"
#include "history.h"

//variabili globali utilizzate per salvare il numero di partizioni, slice e le risorse
int number_of_partitions;
int number_of_slices;
struct list *files_analysis;

// per memorizzare i risultati delle analisi
struct list *last_analysis; // di file_analysis
struct list *logs;          // di history

//variabili per avvire i due processi
pid_t analyzer;
pthread_t analyzer_listener_id;
pid_t report;
pthread_t report_listener_id;

void update_file_analysis(char *file, int char_int, int occurrences)
{
    struct list_iterator *files_analysis_iter = list_iterator_new(last_analysis);
    struct file_analysis *file_analysis;

    while ((file_analysis = (struct file_analysis *)list_iterator_next(files_analysis_iter)))
    {
        if (strcmp(file_analysis->file, file) == 0)
        {
            file_analysis->analysis[char_int] = occurrences;
            break;
        }
    }

    if (file_analysis == NULL)
    {
        struct file_analysis *tmp = file_analysis_new();
        tmp->file = strdup(file);
        tmp->analysis[char_int] = occurrences;

        list_push(last_analysis, tmp);
    }

    list_iterator_delete(files_analysis_iter);
}

void analysis_listener(void *fd_v)
{
    struct history *tmp = history_new();
    last_analysis = tmp->data;
    tmp->timestamp = time(NULL);

    struct list_iterator *files_analysis_iter = list_iterator_new(files_analysis);
    struct file_analysis *file_analysis;
    while ((file_analysis = (struct file_analysis *)list_iterator_next(files_analysis_iter)))
    {
        list_push(tmp->resources, file_analysis);
    }
    list_iterator_delete(files_analysis_iter);

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
            int occurrences;

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

    list_push(logs, tmp);

    close(fd);
}

void print_menu()
{
    //stampa del menù utente
    printf("Comandi disponibili:\n");
    printf("\t- help\n");
    printf("\t- get $parametro\n");
    printf("\t- set $parametro $valore|default\n");
    printf("\t- list\n");
    printf("\t- add $lista_risorse\n");
    printf("\t- del $lista_risorse\n");
    printf("\t- analyze\n");
    printf("\t- report\n");
    printf("\t- import\n");
    printf("\t- export\n");
    printf("\t- exit\n");
    printf("Parametri:\n");
    printf("\t- n (default: 3)\n");
    printf("\t- m (default: 4)\n");
}

int main(int argc, char **argv, char **env)
{
    logs = list_new();

    //inizializzo la lista dei file e le variabili globali con i valori di default
    files_analysis = list_new();
    number_of_partitions = 3;
    number_of_slices = 4;

    // parsing dei parametri della chiamata
    int arg_index = 1; // l'elemento alla posizione 0 e' la stringa di chiamata del programma
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
            char *file = (char *)malloc(sizeof(char) * strlen(argv[arg_index] + 1));
            strcpy(file, argv[arg_index]);

            // TODO verificare se il file e' un file regolare o una directory
            // nel secondo caso, assicurarsi che termini con /
            // TODO stessa cosa anche per il comando add

            struct file_analysis *file_analysis = file_analysis_new();
            file_analysis->file = file;
            list_push(files_analysis, file_analysis);
        }

        arg_index++;
    }

    printf("Sistema di analisi statistiche semplici su caratteri presenti in uno o piu' file\n\n");
    print_menu();

    //variabile utilizzata per gestire il menù
    char *choice = (char *)malloc(sizeof(char) * 100);

    do
    {
        printf("> ");
        fflush(stdout);

        //leggo il comando fino all'invio e ripolisco il canale
        scanf("%[^\n]s", choice);
        while ((getchar()) != '\n')
            ;

        if (strcasecmp(choice, "exit") == 0)
        {
            //se ho digitato exit termino il programma
            break;
        }
        else if (strcasecmp(choice, "get m") == 0)
        {
            //stampo a video il numero di slices
            printf("Il numero di slice è %d\n", number_of_slices);
        }
        else if (strcasecmp(choice, "get n") == 0)
        {
            //stampo a video il numero di partizioni
            printf("Il numero di partition è %d\n", number_of_partitions);
        }
        else if (strncasecmp(choice, "set n", 5) == 0)
        {
            //mi salvo una nuova stringa in cui ci sarò la parte successiva a "set n "
            char *str = &choice[6];
            if (strcasecmp(str, "default") == 0)
            {
                number_of_partitions = 3;
            }
            else if (is_positive_number(str))
            {
                number_of_partitions = atoi(str);
            }
            else
            {
                printf("Il numero deve essere un intero positivo\n");
            }
        }
        else if (strncasecmp(choice, "set m", 5) == 0)
        {
            //mi salvo una nuova stringa in cui ci sarò la parte successiva a "set m "
            char *str = &choice[6];
            if (strcasecmp(str, "default") == 0)
            {
                number_of_slices = 4;
            }
            else if (is_positive_number(str))
            {
                number_of_slices = atoi(str);
            }
            else
            {
                printf("Il numero deve essere un intero positivo\n");
            }
        }
        else if (strcasecmp(choice, "list") == 0)
        {
            // scorro tutta la lista con i file e la stampo
            struct list_iterator *files_analysis_iter = list_iterator_new(files_analysis);
            struct file_analysis *file_analysis;
            while ((file_analysis = (struct file_analysis *)list_iterator_next(files_analysis_iter)))
            {
                printf("%s \n", file_analysis->file);
            }
            list_iterator_delete(files_analysis_iter);
        }
        else if (strncasecmp(choice, "add", 3) == 0)
        {
            // come nel parsing, creo un nuovo file da aggingere alla mia struttura
            char *str = &choice[4];
            char *pch;
            pch = strtok(str, " ");
            while (pch != NULL)
            {
                // TODO controllare le le risorse esistono, in caso segnalare e continuare il loop
                // (per aggiungere ugualmente quelli validi)

                char *file = (char *)malloc(sizeof(char) * (strlen(pch) + 1));
                strcpy(file, pch);

                struct file_analysis *file_analysis = file_analysis_new();
                file_analysis->file = file;
                list_push(files_analysis, file_analysis);
                pch = strtok(NULL, " ");
            }
        }
        else if (strncasecmp(choice, "del", 3) == 0)
        {
            // come nel parsing, creo un nuovo file da aggingere alla mia struttura
            char *str = &choice[4];
            char *pch;
            pch = strtok(str, " ");
            while (pch != NULL)
            {
                char *file = (char *)malloc(sizeof(char) * (strlen(pch) + 1));
                strcpy(file, pch);
                if (!list_delete_file_of_file_analysis(files_analysis, file))
                {
                    printf("La risorsa %s non e' presente nella lista\n", file);
                }
                pch = strtok(NULL, " ");
            }
        }
        else if (strcasecmp(choice, "analyze") == 0)
        {
            int mypipe[2];

            pipe(mypipe);
            analyzer = fork();

            if (analyzer == 0)
            {
                // creazione argomenti per la chiamata a reader
                int number_of_files = files_analysis->lenght;
                char **analyzer_argv = (char **)malloc(sizeof(char *) * (number_of_files + 8)); //: ./analyzer, -n, properties.number_of_partitions, -m, properties.number_of_slices, -r, pid_report, NULL
                int arg_index = 0;

                // primo parametro: nome con cui e' invocato l'eseguibile. non e' importante sia corretto
                analyzer_argv[arg_index++] = "./analyzer";

                // impostazione parametri per quantita' di partition
                analyzer_argv[arg_index++] = "-n";
                char number_of_partiotion_arg[8];
                itoa(number_of_partitions, number_of_partiotion_arg);
                analyzer_argv[arg_index++] = number_of_partiotion_arg;

                // impostazione parametri per quantita' di slices
                analyzer_argv[arg_index++] = "-m";
                char number_of_slices_arg[8];
                itoa(number_of_slices, number_of_slices_arg);
                analyzer_argv[arg_index++] = number_of_slices_arg;

                // impostazione parametri per i file da analizzare
                struct list_iterator *files_analysis_iter = list_iterator_new(files_analysis);
                struct file_analysis *file_analysis;
                while ((file_analysis = (struct file_analysis *)list_iterator_next(files_analysis_iter)))
                {
                    analyzer_argv[arg_index++] = file_analysis->file;
                }
                list_iterator_delete(files_analysis_iter);

                // null-terminated vector
                analyzer_argv[arg_index] = NULL;

                // redirezione dell'output nella write-end della pipe
                dup2(mypipe[1], STDOUT_FILENO);
                close(mypipe[0]);
                close(mypipe[1]);

                execve("bin/analyzer", analyzer_argv, env);
            }
            else
            {
                // avvio thread per eseguire il listener
                close(mypipe[1]);
                pthread_create(&analyzer_listener_id, NULL, (void *)analysis_listener, (void *)&mypipe[0]);
                pthread_join(analyzer_listener_id, NULL);
            }
        }
        else if (strcasecmp(choice, "history") == 0)
        {
            int index = 0;

            struct list_iterator *logs_iter = list_iterator_new(logs);
            struct history *history;
            while ((history = (struct history *)list_iterator_next(logs_iter)))
            {
                printf("[%d]\teseguito %s", index, ctime(&history->timestamp)); // gia' contiene \n
                printf("\tlista delle risorse:\n");

                // itera lista risorse
                struct list_iterator *files_analysis_iter = list_iterator_new(history->resources);
                struct file_analysis *file_analysis;
                while ((file_analysis = (struct file_analysis *)list_iterator_next(files_analysis_iter)))
                {
                    printf("\t\t- %s\n", file_analysis->file);
                }
                list_iterator_delete(files_analysis_iter);

                index++;
            }
        }
        else if (strcasecmp(choice, "report") == 0)
        {

            // TODO acquisire l'id della history da effettuare
            // estralo da logs
            // e utilizzare il suo data invece di last_analysis
            // per l'invio dei dati (dentro else)

            int mypipe[2];

            pipe(mypipe);

            //printf("1) maiuscole e minuscole\n");
            printf("1) Resoconto generale\n");
            printf("2) Caratteri stampabili\n");
            printf("3) Lettere\n");
            printf("4) Spazi, numeri e punteggiatura\n");
            printf("5) all\n");
            char **report_argv = (char **)malloc(sizeof(char *) * (501));
            int arg_index;
            int flags;
            do
            {
                printf("Inserire un numero per indicare una scelta: ");
                scanf("%d", &flags);
                while ((getchar()) != '\n')
                    ;

                arg_index = 0;

                report_argv[arg_index++] = "./report";
                report_argv[arg_index++] = "file";
                report_argv[arg_index++] = "allchars";
                report_argv[arg_index++] = "-ls";


                if (flags == 1)
                {
                    report_argv[arg_index++] = "-p";
                    report_argv[arg_index++] = "-np";
                    report_argv[arg_index++] = "-lett";
                    report_argv[arg_index++] = "-num";
                    report_argv[arg_index++] = "-punt";
                    report_argv[arg_index++] = "-sp";
                }
                else if (flags == 2)
                {
                    report_argv[arg_index++] = "-p";
                    report_argv[arg_index++] = "-lett";
                    report_argv[arg_index++] = "-M";
                    report_argv[arg_index++] = "-m";
                    report_argv[arg_index++] = "-num";
                    report_argv[arg_index++] = "-punt";
                    report_argv[arg_index++] = "-sp";
                }
                else if (flags == 3)
                {
                    report_argv[arg_index++] = "-p";
                    report_argv[arg_index++] = "-lett";
                    report_argv[arg_index++] = "-allM";
                    report_argv[arg_index++] = "-allm";
                }
                else if (flags == 4)
                {
                    report_argv[arg_index++] = "-p";
                    report_argv[arg_index++] = "-sp";
                    report_argv[arg_index++] = "-allnum";
                    report_argv[arg_index++] = "-allpunt";
                }
                else if (flags == 5)
                {
                    report_argv[arg_index++] = "-allch";
                }
                else
                {
                    printf("Numero invalido!\n");
                }
            } while (flags < 1 || flags > 5);
            report_argv[arg_index] = NULL;

            report = fork();

            if (report == 0)
            {
                // redirezione dell'input nella write-end della pipe
                dup2(mypipe[0], STDIN_FILENO);
                close(mypipe[0]);
                close(mypipe[1]);

                execve("bin/report", report_argv, env);
            }
            else
            {
                close(mypipe[0]);
                FILE *stream = fdopen(mypipe[1], "w");

                struct list_iterator *files_analysis_iter = list_iterator_new(last_analysis);
                struct file_analysis *file_analysis;
                while ((file_analysis = (struct file_analysis *)list_iterator_next(files_analysis_iter)))
                {
                    int char_int = 0;
                    while (char_int < 128)
                    {
                        if (file_analysis->analysis[char_int] > 0)
                        {
                            fprintf(stream, "%s:%d:%lu\n", file_analysis->file, char_int, file_analysis->analysis[char_int]);
                        }
                        char_int++;
                    }
                }

                fclose(stream);
                close(mypipe[1]);

                waitpid(report, NULL, 0);
            }
        }
        else if (strcasecmp(choice, "help") == 0)
        {
            print_menu();
        }
        else
        {
            //la scelta non è accettata o digitata erroneamente e lo comunico all'utente
            printf("Comando sconosciuto!\n");
        }
    } while (true);

    return 0;
}
