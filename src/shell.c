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

// aggiorna (incrementa) le occorrenze di `occurrences` del carattere `char_int`
// per il file `file` e le salva nella struttura last_analysis

void update_file_analysis(char *file, int char_int, unsigned long occurrences)
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

// funzione eseguita da thread avviato quando viene richiamato l'analyzer
// legge l'output di un processo analyze sino alla sua conclusione,
// estrapola informazioni e aggiorna le occorrenze indicate

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
            unsigned long occurrences;

            if (file_analysis_parse_line(a_line, &file, &char_int, &occurrences))
            {
                // aggiornamento occorrenze
                update_file_analysis(file, char_int, occurrences);
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

//stampa del menù utente
void print_menu()
{
    printf("Comandi disponibili:\n");
    printf("- `help`: stampa a video un breve riassunto dei comandi possibili\n");
    printf("- `get $var`: restituisce il valore della variabile indicata (`m` o `n`)\n");
    printf("- `set $var $val|default`: imposta il valore della variabile indicata (`m` o `n`) con il valore numerico (intero positivo) di `val`, oppure con il valore di default (`m` = 3, `n` = 4)\n");
    printf("- `list`: stampa a video le risorse da analizzare\n");
    printf("- `add $file_list`: aggiunge i file specificati tra le risorse da analizzare\n");
    printf("- `del $file_file`: rimuove i file specificati dalle risorse da analizzare\n");
    printf("- `analyze`: avvia l'analisi sulle risorse impostate con i valori di `m` e di `n` impostati. I risultati sono salvati internamente\n");
    printf("- `history`: stampa a video uno storico delle analisi effettuate, visualizzando data e ora dell'esecuzione e la lista delle risorse incluse\n");
    printf("- `report [$history_id]`: avvia il report sui dati presenti nel record di history indicato da `history_id`. Se non espresso, la esegue sull'ultima analisi effettuata\n");
    printf("- `import $file`: importa i risultati delle analisi precedentemente esportate, assieme alla lista di risorse ed alla data ed ora di esecuzione\n");
    printf("- `export $history_id $file`: esporta i risultati della analisi, assieme alla lista di risorse ed alla data ed ora di esecuzione\n");
    printf("- `exit`: chiude la shell\n");
    }

int main(int argc, char **argv, char **env)
{
    // controllo esistenza degli eseguibili chiamati da execve
    if (is_executable("bin/report") != 1)
    {
        fprintf(stderr, "Non e' possibile trovare l'eseguibile bin/report\n");
        return;
    }
    if (is_executable("bin/analyzer") != 1)
    {
        fprintf(stderr, "Non e' possibile trovare l'eseguibile bin/analyzer\n");
        return;
    }

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
                fprintf(stderr, "Il parametro n non e' passato\nValore di default: %d\n", number_of_partitions);
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
                fprintf(stderr, "Il parametro m non e' passato\nValore di default: %d\n", number_of_slices);
            }
        }
        else
        {
            //alloco la memoria sufficiente a memorizzare il nome delfile + il carattere di terminazione
            char *file = (char *)malloc(sizeof(char) * strlen(argv[arg_index] + 1));
            strcpy(file, argv[arg_index]);
            //controllo se la risora passata esiste o meno
            int ex = is_directory(file); //avrà valore -1 se non esiste, altro se è un file o una directory

            if (ex == -1)
            {
                // non esiste
                fprintf(stderr, "La risorsa %s non esiste\n", file);
                arg_index++;
                continue;
            }

            //controllo che la risorsa non sia già presente nella lista
            if (is_string_present_in_list(files_analysis, file))
            {
                //se la risorsa già presente viene comunicato e non viene aggiunta alla lista delle risorse
                fprintf(stderr, "La risorsa %s è già presente\n", file);
                arg_index++;
                continue;
            }


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

        //leggo il comando fino all'invio e ripulisco il canale
        scanf("%[^\n]s", choice);
        while ((getchar()) != '\n')
            ;

        //inizio il parsing della stringa letta
        char *cmd;
        cmd = strtok(choice, " ");

        if(cmd==NULL)
        {
            printf("Comando omesso!\n");
        }
        else if (strcasecmp(cmd, "exit") == 0)
        {
            //interruzione del programma
            break;
        }
        else if (strcasecmp(cmd, "get") == 0)
        {
            char *var = strtok(NULL, " ");
            if(var==NULL){
                fprintf(stderr, "Parametro omesso\n");
                continue;
            }

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
            char *var = strtok(NULL, " "); //il primo token indica su che variabile devo andare ad agire
            if(var==NULL){
                fprintf(stderr, "Parametro omesso\n");
                continue;
            }

            char *val = strtok(NULL, " "); //il secondo token indica il valore da assegnare alla variabile
            if(val==NULL){
                fprintf(stderr, "Valore omesso\n");
                continue;
            }

            //verifico che la variabile sia n o m
            if (strcasecmp(var, "m") != 0 && strcasecmp(var, "n") != 0)
            {
                printf("%s non e' una variabile conosciuta\n", var);
                continue;
            }

            //è stato richiesto di assegnare il valore di default
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
            else if (is_positive_number(val))   //e' stato richiesto di assegnare un valore di cui si verifica la validita'(> 0)
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
            // free(val);
            // free(var);
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
            if(new_file==NULL){
                fprintf(stderr, "Lista risorse omesse\n");
                continue;
            }

            //finchè ci sono token con nomi di risorse da allocare
            while (new_file != NULL)
            {

                //alloco la memoria sufficiente a memorizzare il nome delfile + il carattere di terminazione
                char *file = (char *)malloc(sizeof(char) * (strlen(new_file) + 1));
                strcpy(file, new_file);

                //vado al token successivo
                new_file = strtok(NULL, " ");

                //controllo se la risora passata esiste o meno
                int ex = is_directory(file); //avrà valore -1 se non esiste, altro se è un file o una directory

                if (ex == -1)
                {
                    // non esiste
                    fprintf(stderr, "La risorsa %s non esiste\n", file);
                    continue;
                }

                //controllo che la risorsa non sia già presente nella lista
                if (is_string_present_in_list(files_analysis, file))
                {
                    //se la risorsa già presente viene comunicato e non viene aggiunta alla lista delle risorse
                    fprintf(stderr, "La risorsa %s è già presente\n", file);
                    continue;
                }

                struct file_analysis *file_analysis = file_analysis_new();
                file_analysis->file = file;
                list_push(files_analysis, file_analysis);
            }
        }
        else if (strcasecmp(cmd, "del") == 0)
        {
            // creo un file per capire quale eliminare dalla mia struttura
            char *old_file;
            old_file = strtok(NULL, " "); //il nome del file si troverà nel primo token
            if(old_file==NULL){
                fprintf(stderr, "Lista risorse omesse\n");
                continue;
            }

            //finchè ci sono token con nomi di risorse da cancellare
            while (old_file != NULL)
            {
                //alloco la memoria sufficiente a memorizzare il nome delfile + il carattere di terminazione
                char *file = (char *)malloc(sizeof(char) * (strlen(old_file) + 1));
                strcpy(file, old_file);
                //provo ad eleiminare la risorsa
                if (!list_delete_file_of_file_analysis(files_analysis, file))
                {
                    //segnalo se non è presente
                    fprintf(stderr, "La risorsa %s non e' presente nella lista\n", file);
                }
                //vado al token successivo
                old_file = strtok(NULL, " ");
            }
        }
        else if (strcasecmp(cmd, "analyze") == 0)
        {
            // non avviarlo se non ci sono risorse
            if (files_analysis->lenght == 0)
            {
                fprintf(stderr, "Non ci sono risorse da analizzare\n");
            }
            else
            {
                //creo la pipe di comunicazione da usare con il processo analyzer
                int mypipe[2];

                //verifico che si possa creare la pipe
                if (pipe(mypipe) == -1)
                {
                    //se non si può creare vado avanti con il programma segnalando un errore di apertura
                    fprintf(stderr, "Fallimento nella creazione della pipe/n");
                }
                else
                {

                    //provo a fare la fork e se fallisce riprovo a farla finchè non si liberano le risorse
                    bool waiting = false;
                    while ((analyzer = fork()) < 0)
                    {
                        if (waiting == false)
                        {
                            fprintf(stderr, "In attesa di risorse analyzer\n");
                            waiting = true;
                        }
                        usleep(100);
                    }

                    if (analyzer == 0)
                    {
                        // creazione argomenti per la chiamata a analyzer
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
                        dup2(mypipe[1], STDOUT_FILENO); //TODO gestire dup2
                        close(mypipe[0]);
                        close(mypipe[1]);

                        execve("bin/analyzer", analyzer_argv, env);
                    }
                    else
                    {
                        // avvio thread per eseguire il listener
                        close(mypipe[1]);

                        //provo ad avvire il thread e se fallisce riprovo a farlo finchè non si liberano le risorse
                        bool waiting = false;
                        while (pthread_create(&analyzer_listener_id, NULL, (void *)analysis_listener, (void *)&mypipe[0]))
                        {
                            if (waiting == false)
                            {
                                fprintf(stderr, "In attesa di risorse thread\n");
                                waiting = true;
                            }
                            usleep(100);
                        }

                        pthread_join(analyzer_listener_id, NULL);
                    }
                }
            }
        }
        else if (strcasecmp(cmd, "history") == 0)
        {
            int index = 1;

            //itero tutte le history con le analyze fatte in precedenza e stampo un riepilogo
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

            list_iterator_delete(logs_iter);
        }
        else if (strcasecmp(cmd, "report") == 0)
        {
            //verifico di avere dei file che il report possa leggere ed analizzare
            //ovvero, verifico che la history degli analyze non sia vuota
            if (logs->lenght == 0)
            {
                fprintf(stderr, "La history e' vuota\n");
                continue;
            }

            char *str = strtok(NULL, " "); //token in cui è presente l'id della history da eseguire

            int logs_index = -1;

            //se il token non è presente, allora vuol dire che devo lavorare sull'ultima history salvata
            if (str == NULL)
            {
                logs_index = logs->lenght;
            }
            else if (is_positive_number(str))
            {
                //altrimenti provo a controllare se l'id passato èvalido
                logs_index = atoi(str);
                if (logs_index > logs->lenght)
                    fprintf(stderr, "History_id invalido\n");
            }
            else
            {
                fprintf(stderr, "Il numero deve essere un intero positivo\n");
            }
            if (logs_index > 0 && logs_index <= logs->lenght)
            {
                //mi creo una struttura per salvare una lista che punterà all'analisi da passare al report
                struct list *selected_analysis = list_new();

                //itero logs_index volte per selezionare l'analisi desiderata
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

                if (pipe(mypipe) == -1)
                {
                    //se non si può creare vado avanti con il programma segnalando un errore di apertura
                    fprintf(stderr, "Fallimento nella creazione della pipe/n");
                }
                else
                {
                    //visualizze le modalità con cui l'utente può avvire il report 
                    printf("1) Resoconto generale\n");
                    printf("2) Caratteri stampabili\n");
                    printf("3) Lettere\n");
                    printf("4) Spazi, numeri e punteggiatura\n");
                    printf("5) all\n");

                    //struttura dati usata per creare la chiamata
                    char **report_argv = (char **)malloc(sizeof(char *) * (40));

                    int arg_index;
                    int flags = -1;
                    do
                    {
                        printf("Inserire un numero per indicare una scelta: ");
                        scanf("%d", &flags);
                        while ((getchar()) != '\n')
                            ;

                        arg_index = 0;

                        //inizializzazione degli argomenti di chiamata
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
                    
                    //carattere di terminazione
                    report_argv[arg_index] = NULL;

                    //provo a fare la fork e se fallisce riprovo a farla finchè non si liberano le risorse
                    bool waiting = false;
                    while ((report = fork()) < 0)
                    {
                        if (waiting == false)
                        {
                            fprintf(stderr, "In attesa di risorse\n");
                            waiting = true;
                        }
                        usleep(100);
                    }

                    if (report == 0)
                    {
                        // redirezione dell'input nella front-end della pipe
                        dup2(mypipe[0], STDIN_FILENO); //TODO errori dup2
                        close(mypipe[0]);
                        close(mypipe[1]);

                        execve("bin/report", report_argv, env);
                    }
                    else
                    {
                        close(mypipe[0]);

                        //TODO gestione apertura stream

                        //apro lo stream per scrivere nella pipe
                        FILE *stream = fdopen(mypipe[1], "w");

                        //itero e scrivo tutti i dati contenuti nell'analisi selezionata
                        //così che il processo reader la possa leggere ed elaborare
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
        }
        else if (strcasecmp(cmd, "help") == 0)
        {
            //se richiesto dall'utente stampa il menù con le istruzioni che si possono eseguire
            print_menu();
        }
        else if (strcasecmp(cmd, "import") == 0)
        {
            //TODO cosa fare se passiamo un file con una formattazione o un formato invalido?

            char *file = strtok(NULL, " ");//token contenente il nome del file da importare

            //se il token è vuoto, imposto "saved.txt" come file di default
            //questo perchè si verifica una situazione analoga in export
            if(file==NULL){
                fprintf(stderr,"File omesso\n");
                continue; 
            }

            // controllo esistenza file
            int ex = is_directory(file);

            if (ex == -1)
            {
                // non esiste 
                fprintf(stderr,"%s non esiste\n", file);
                continue;
            }
            else if (ex == 1)
            {
                // una directory
                fprintf(stderr,"%s e' una directory\n", file);
                continue;
            }

            struct history *imp = history_new();
            list_push(logs, imp);
            last_analysis = imp->data;

            // apertura file
            FILE *stream = fopen(file, "r");
            if (stream == NULL)
            {
                fprintf(stderr, "Impossibile aprire lo stream di lettura per il file\n");
            }
            else
            {
                //leggo riga per riga e salvo le informazioni in una nuova history
                char line[LINE_SIZE];
                bool timestamp = false;
                bool body = false;

                //la lettura è eseguita seguendo una formattazione stabilita
                while (fscanf(stream, "%s", line) != EOF)
                {
                    if (!timestamp)
                    {
                        timestamp = true;
                        imp->timestamp = atoi(line);
                        continue;
                    }

                    if (strcmp(line, "---") == 0)
                    {
                        body = true;
                        continue;
                    }

                    if (timestamp && !body)
                    {
                        //risorsa
                        struct file_analysis *res = file_analysis_new();
                        res->file = strdup(line);

                        list_push(imp->resources, res);
                        continue;
                    }

                    //lettura delle informazioni
                    char *file;
                    int char_int;
                    unsigned long occurrences;

                    //aggiornamento della struttura in cui sono salvate le occorrenze
                    if (file_analysis_parse_line(line, &file, &char_int, &occurrences))
                    {
                        update_file_analysis(file, char_int, occurrences);
                    }
                }
            }

            fclose(stream);
        }
        else if (strcasecmp(cmd, "export") == 0)
        {
            
            char *history = strtok(NULL, " "); //token contenente l'id della history da esportare
            int logs_index = -1;

            //se il token è null esport di default l'utliama history salvata
            //ovviamente, se la lista non è vuota
            if (history == NULL)
            {
                fprintf(stderr,"History omessa\n");
                continue;
            }
            else if (is_positive_number(history))
            {
                //altrimenti verifico che l'id passato sia valido
                logs_index = atoi(history);
                if (logs_index > logs->lenght)
                {
                    printf("History_id invalido\n");
                    continue;
                }
            }
            else
            {
                printf("Il numero deve essere un intero positivo\n");
            }


            char *file = strtok(NULL, " ");//token contenente il nome del file in cui effettuare l'export
            if (file == NULL)
            {
                fprintf(stderr,"File omesso\n");
                continue;
            }
            
            //apro lo stream verso il file in cui esportare le cose
            FILE *export_file = fopen(file, "w");

            //se non riesco ad aprire il file segnalo l'errore e non scrive nel file
            if (export_file == NULL)
            {
                fprintf(stderr, "Impossibile aprire lo stream di scrittura per il file\n");
            }
            else
            {
                //salvo sul file aperto il timestap, la lista delle risorse e la loro analisi
                if (logs_index > 0 && logs_index <= logs->lenght && export_file != NULL)
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

                    fprintf(export_file, "%ld\n", selected_time);

                    struct list *selected_files = tmp->resources;

                    struct list_iterator *files_iter = list_iterator_new(selected_files);
                    struct file_analysis *file_selceted;
                    while ((file_selceted = (struct file_analysis *)list_iterator_next(files_iter)))
                    {
                        fprintf(export_file, "%s\n", file_selceted->file);
                    }
                    list_iterator_delete(files_iter);

                    //separatore di risorse e prima occorenza delle analisi
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

                    list_iterator_delete(files_analysis_iter);
                }

                fclose(export_file);
            }
        }
        else 
        {
            printf("Comando sconosciuto!\n");
        }

    } while (true);

    free(choice);

    return 0;
}
