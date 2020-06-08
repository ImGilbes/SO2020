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
#include "fs.h"

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
        char *file = (char *)malloc(sizeof(char) * strlen(file_analysis->file + 1));
        strcpy(file, file_analysis->file);
        struct file_analysis *file_analysis_tmp = file_analysis_new();
        file_analysis_tmp->file = file;
        list_push(tmp->resources, file_analysis_tmp);
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
    printf("\t- history\n");
    printf("\t- report $history_id|last\n");
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
            // TODO se gia' esiste nella lista, cheffamo?
            // TODO se non esistono le risorse, le aggiungiamo?
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

        char *cmd;
        cmd = strtok(choice, " ");

        if (strcasecmp(cmd, "exit") == 0)
        {
            break;
        }
        else if (strcasecmp(cmd, "get") == 0)
        {
            char *var = strtok(NULL, " ");

            if (strcasecmp(var, "m") == 0)
            {
                printf("%d\n", number_of_slices);
            }
            else if (strcasecmp(var, "n") == 0)
            {
                printf("%d\n", number_of_partitions);
            }
            else
            {
                printf("%s non e' una variabile conosciuta\n", var);
            }
        }
        else if (strcasecmp(cmd, "set") == 0)
        {
            char *var = strtok(NULL, " ");
            char *val = strtok(NULL, " ");

            if (strcasecmp(var, "m") != 0 && strcasecmp(var, "n") != 0)
            {
                printf("%s non e' una variabile conosciuta\n", var);
                continue;
            }

            if (strcasecmp(val, "default") == 0)
            {
                if (strcasecmp(var, "m") == 0)
                {
                    number_of_slices = 4;
                }
                else
                {
                    number_of_partitions = 3;
                }
            }

            if (is_positive_number(val))
            {
                if (strcasecmp(var, "m") == 0)
                {
                    number_of_slices = atoi(val);
                }
                else
                {
                    number_of_partitions = atoi(val);
                }
            }
            else
            {
                printf("%s non e' un valore valido\n", val);
            }
        }
        else if (strcasecmp(cmd, "list") == 0)
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
        else if (strcasecmp(cmd, "add") == 0)
        {
            // come nel parsing, creo un nuovo file da aggingere alla mia struttura
            char *new_file;
            new_file = strtok(NULL, " ");
            while (new_file != NULL)
            {
                // TODO controllare le le risorse esistono, in caso segnalare e continuare il loop
                // (per aggiungere ugualmente quelli validi)

                char *file = (char *)malloc(sizeof(char) * (strlen(new_file) + 1));
                strcpy(file, new_file);

                struct file_analysis *file_analysis = file_analysis_new();
                file_analysis->file = file;
                list_push(files_analysis, file_analysis);
                new_file = strtok(NULL, " ");
            }
        }
        else if (strcasecmp(cmd, "del") == 0)
        {
            // come nel parsing, creo un nuovo file da aggingere alla mia struttura
            char *old_file;
            old_file = strtok(NULL, " ");
            while (old_file != NULL)
            {
                char *file = (char *)malloc(sizeof(char) * (strlen(old_file) + 1));
                strcpy(file, old_file);
                if (!list_delete_file_of_file_analysis(files_analysis, file))
                {
                    printf("La risorsa %s non e' presente nella lista\n", file);
                }
                old_file = strtok(NULL, " ");
            }
        }
        else if (strcasecmp(cmd, "analyze") == 0)
        {
            // TODO non avviarlo se non ci sono risorse
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
        else if (strcasecmp(cmd, "history") == 0)
        {
            int index = 1;

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
        else if (strcasecmp(cmd, "report") == 0)
        {
            // TODO di default, senza argomenti, prende l'ultimo history
            char *str = strtok(NULL, " ");
            int logs_index = -1;

            if (str == NULL)
            {
                logs_index = logs->lenght;
            }
            else if (is_positive_number(str))
            {
                logs_index = atoi(str);
                if (logs_index <= logs->lenght)
                    printf("History_id invalido\n");
            }
            else
            {
                printf("Il numero deve essere un intero positivo\n");
            }

            if (logs_index > 0 && logs_index <= logs->lenght)
            {
                struct list *selected_analysis = list_new();

                struct list_iterator *logs_iter = list_iterator_new(logs);
                struct history *tmp;
                int i;
                for (i = 1; i <= logs_index; i++)
                {
                    tmp = (struct history *)list_iterator_next(logs_iter);
                }

                selected_analysis = tmp->data;

                list_iterator_delete(logs_iter);

                int mypipe[2];

                pipe(mypipe);

                char **report_argv = (char **)malloc(sizeof(char *) * (20));
                int arg_index;
                char *flags = (char *)malloc(sizeof(char) * 40);

                printf("-ls) stampa lista file e totale caratteri\n");
                printf("-p) totale caratteri stampabili\n");
                printf("-np) totale caratteri non stampabili\n");
                printf("-lett) totale lettere\n");
                printf("-punt) totale punteggiatura\n");
                printf("-allpunt) totale punteggiatura con dati per ciascun singolo carattere di punteggiatura\n");
                printf("-M) totale maiuscole\n");
                printf("-allM) totale maiuscole con dati per ogni lettera maiuscola\n");
                printf("-m) totale minuscole\n");
                printf("-allm) totale minuscole con dati per ogni lettera minuscola\n");
                printf("-sp) totale spazi\n");
                printf("-num) totale numeri\n");
                printf("-allnum) totale numeri con dati per ogni numero\n");
                printf("-allch) dati specifici per ogni carattere\n");
                printf("Inserire con quali modalità avviare il report (esempio: -ls -np -M): ");
                //leggo il comando fino all'invio e ripolisco il canale
                scanf("%[^\n]s", flags);
                while ((getchar()) != '\n')
                    ;

                arg_index = 0;

                report_argv[arg_index++] = "./report";
                report_argv[arg_index++] = "file";
                report_argv[arg_index++] = "allchars"; // TODO alternativa ponly

                char *pch;
                pch = strtok(flags, " ");
                while (pch != NULL)
                {
                    report_argv[arg_index++] = pch;

                    pch = strtok(NULL, " ");
                }

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

                    struct list_iterator *files_analysis_iter = list_iterator_new(selected_analysis);
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
        }
        else if (strcasecmp(cmd, "help") == 0)
        {
            print_menu();
        }
        else if (strcasecmp(cmd, "import") == 0)
        {
            char *file = strtok(NULL, " ");
            
            // controllo esistenza file
            int ex = is_directory(file);

            if (ex == -1) {
                // non esiste
                printf("%s non esiste\n", file);
                continue;
            } else if (ex == 1) {
                // una directory
                printf("%s e' una directory\n", file);
                continue;
            }

            struct history *imp = history_new();
            list_push(logs, imp);
            last_analysis = imp->data; //MANCAVA QUESTO LOL XD

            // apertura file
            FILE *stream = fopen(file, "r");
            char line[LINE_SIZE];
            bool timestamp = false;
            bool body = false;

            while (fscanf(stream, "%[^\n]s", line) != EOF) {
                printf("%s\n", line);

                if (!timestamp) {
                    timestamp = true;
                    imp->timestamp = atoi(line);
                    continue;
                }
                
                if (strcmp(line, "---") == 0) {
                    body = true;
                    continue;
                }

                if (timestamp && !body) {
                    //risorsa
                    struct file_analysis *res = file_analysis_new();
                    res->file = strdup(line);

                    list_push(imp->resources, res);
                    continue;
                }

                //lettura delle informazioni
                char *file;
                int char_int;
                int occurrences;

                if (file_analysis_parse_line(line, &file, &char_int, &occurrences))
                {
                    // aggiornamento occorrenze
                    update_file_analysis(file, char_int, occurrences);
                    free(file);
                }
            }

            fclose(stream);
        }
        else if (strcasecmp(cmd, "export") == 0)
        {
            char *history = strtok(NULL, " ");
            int logs_index = -1;

            if (history == NULL)
            {
                logs_index = logs->lenght;
            }
            else if (is_positive_number(history))
            {
                logs_index = atoi(history);
                if (logs_index > logs->lenght)
                    printf("History_id invalido\n");
            }
            else
            {
                printf("Il numero deve essere un intero positivo\n");
            }

            char *file = strtok(NULL, " ");
            if (file == NULL)
            {
                file="saved.txt";
            }

            FILE *export_file = fopen(file, "w");

            if (logs_index > 0 && logs_index <= logs->lenght && export_file!=NULL)
            {

                struct list_iterator *logs_iter = list_iterator_new(logs);
                struct history *tmp;
                int i;
                for (i = 1; i <= logs_index; i++)
                {
                    tmp = (struct history *)list_iterator_next(logs_iter);
                }

                list_iterator_delete(logs_iter);

                time_t selected_time = tmp->timestamp;

                fprintf(export_file,"%s", ctime(&selected_time));
                
                struct list *selected_files = tmp->resources;

                struct list_iterator *files_iter = list_iterator_new(selected_files);
                struct file_analysis *file_selceted;
                while ((file_selceted = (struct file_analysis *)list_iterator_next(files_iter)))
                {
                    fprintf(export_file, "%s\n", file_selceted->file);
                }
                list_iterator_delete(files_iter);

                fprintf(export_file, "---\n");

                struct list *selected_analysis = tmp->data;

                struct list_iterator *files_analysis_iter = list_iterator_new(selected_analysis);
                struct file_analysis *file_analysis;
                while ((file_analysis = (struct file_analysis *)list_iterator_next(files_analysis_iter)))
                {
                    int char_int = 0;
                    while (char_int < 128)
                    {
                        if (file_analysis->analysis[char_int] > 0)
                        {
                            fprintf(export_file, "%s:%d:%lu\n", file_analysis->file, char_int, file_analysis->analysis[char_int]);
                        }
                        char_int++;
                    }
                }

                fclose(export_file);

                list_iterator_delete(files_analysis_iter);

                
            }

        }
        else
        {
            printf("Comando sconosciuto!\n");
        }

    } while (true);

    return 0;
}
