#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <ctype.h>

#include "list.h"
#include "file_analysis.h"
#include "itoa.h"
#include "utilities.h"

//variabili globali utilizzate per salvare il numero di partizioni, slice e le risorse
int number_of_partitions;
int number_of_slices;
struct list *files_analysis;

//variabili per avvire i due processi
pid_t analyzer;
pthread_t analyzer_listener_id;
pid_t report;
pthread_t report_listener_id;

void reader_listener(void *fd_v)
{
}

int main(int argc, char **argv, char **env)
{

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

            struct file_analysis *file_analysis = file_analysis_new();
            file_analysis->file = file;
            list_push(files_analysis, file_analysis);
        }

        arg_index++;
    }

    //variabile utilizzata per gestire il menù
    char *choice = (char *)malloc(sizeof(char) * 100);
    printf("\nSistema di analisi statistiche semplici su caratteri presenti in uno o piu' file\n");

    do
    {
        //stampa del menù utente
        printf("\n\nComandi disponibili:\n");
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
        printf("Inserire un comando: ");

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
            printf("Il numero di slice è %d", number_of_slices);
        }
        else if (strcasecmp(choice, "get n") == 0)
        {
            //stampo a video il numero di partizioni
            printf("Il numero di partition è %d", number_of_partitions);
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
                printf("Formattazione invalida");
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
                printf("Formattazione invalida");
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
	        pch = strtok (str, " ");
	        while ( pch!=NULL ){
            	char *file = (char *)malloc(sizeof(char) * (strlen(pch)+1));
            	strcpy(file, pch);

            	struct file_analysis *file_analysis = file_analysis_new();
            	file_analysis->file = file;
            	list_push(files_analysis, file_analysis);
		        pch = strtok(NULL," ");
	        }
	        printf("Ho aggiunto le risorse alla lista!");
        }
        else if (strncasecmp(choice, "delete", 3) == 0)
        {
            // come nel parsing, creo un nuovo file da aggingere alla mia struttura
            char *str = &choice[7];
            char *pch;
            pch = strtok (str, " ");
            while ( pch!=NULL ){

                char *file = (char *)malloc(sizeof(char) * (strlen(pch)+1));
                strcpy(file, pch);
                printf("%s \n",file);
                if(list_delete_file_of_file_analysis(files_analysis, file)){
                    printf("File eliminato!");			
                }else{
                    printf("File non trovato!");		
                }
                pch = strtok(NULL," ");
	        }

        }
        else if (strcasecmp(choice, "analyze") == 0)
        {

            int mypipe[2];

            pipe(mypipe);
            //analyzer = fork();

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
                int i;
                for (i = 0; i < arg_index; i++)
                    printf("%s ", analyzer_argv[i]);
                //fflush(stdout);

                // redirezione dell'output nella write-end della pipe
                // dup2(mypipe[1], STDOUT_FILENO);
                //close(mypipe[0]);
                //close(mypipe[1]);

                //execve("bin/analyzer", analyzer_argv, env);
            }
            else
            {
                // avvio thread per eseguire il listener
                //close(mypipe[1]);
                // pthread_create(analyzer_listener_id, NULL, (void *)reader_listener, (void *)mypipe[0]);
            }
        }
        else if (strcasecmp(choice, "report") == 0)
        {
            int mypipe[2];

            //pipe(mypipe);
            //report = fork();

            if (report == 0)
            {
                printf("\t\t1) maiuscole e minuscole\n");
                printf("\t\t2) numeri\n");
                printf("\t\t3) punteggiatura e spazi\n");
                printf("\t\t4) caratteri non stampabili\n");
                printf("\t\t5) all\n");
                char **report_argv = (char **)malloc(sizeof(char *) * (4));
                int arg_index;
                int flags;
                do
                {
                    printf("\tInserire un numero per indicare una scelta: ");
                    scanf("%d", &flags);
                    while ((getchar()) != '\n')
                        ;

                    arg_index = 0;

                    report_argv[arg_index++] = "./report";

                    if (flags == 1)
                    {
                        report_argv[arg_index++] = "-m";
                        report_argv[arg_index++] = "-M";
                    }
                    else if (flags == 2)
                    {
                        report_argv[arg_index++] = "-num";
                    }
                    else if (flags == 3)
                    {
                        report_argv[arg_index++] = "-punt";
                        report_argv[arg_index++] = "-sp";
                    }
                    else if (flags == 4)
                    {
                        report_argv[arg_index++] = "-np";
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

                int i;
                for (i = 0; i < arg_index; i++)
                    printf("%s ", report_argv[i]);

                // redirezione dell'input nella write-end della pipe
                //dup2(mypipe[1], STDIN_FILENO);
                //close(mypipe[0]);
                //close(mypipe[1]);

                //execve("bin/report", report_argv, env);
            }
            else
            {
                //close(mypipe[1]);
                //pthread_create(&report, NULL, (void *) writer, (void *) &mypipe[0]);
                //pthread_join(report, NULL);
            }
        }
        else
        {
            //la scelta non è accettata o digitata erroneamente e lo comunico all'utente
            printf("Comando invalido!\n");
        }
    } while (strcasecmp(choice, "exit") != 0);

    return 0;
}
