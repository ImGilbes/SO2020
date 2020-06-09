#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "list.h"
#include "bool.h"
#include "itoa.h"
#include "settings.h"
#include "file_analysis.h"

#define delim ":"

void updateList(struct list *l, char *fileName);
void printFileList(struct list *l);
void printPunt(unsigned long *count, unsigned long totpunt, unsigned long tot, bool all);
void printNum(unsigned long *count, unsigned long totnum, unsigned long tot, bool all);
void printMaiusc(unsigned long *count, unsigned long totM, unsigned long tot, bool all);
void printMinusc(unsigned long *count, unsigned long totmin, unsigned long tot, bool all);
void printAll(unsigned long *count, unsigned long tot);
void to_string(char c, char *s);
int numOfDigits(int n);

int main(int argc, char **argv)
{
    struct list *fileList = list_new();
    unsigned long count[128];
    int i;
    for (i = 0; i < 128; i++)
    {
        count[i] = 0;
    }

    int source = STDIN_FILENO;
    char fifo_path[50];

    if (strcmp(argv[1], "npipe") == 0)
    {
        strcpy(fifo_path, "/tmp/report_fifo");
        fprintf(stderr, "Avviare l'analyzer con il parametro -r\n");
        //itoa(getpid(), fifo_path + (strlen(fifo_path)));
        //fprintf(stderr, "La fifo si trova in %s\n", fifo_path);
        mkfifo(fifo_path, 0666);
        source = open(fifo_path, O_RDONLY);
        if(source == -1)
        {
            fprintf(stderr, "Errore apertura fifo\n");
        }
    }
     
    char a_char[2] = {'\0', '\0'};
    char a_line[LINE_SIZE] = {'\0'};
    int bytes;

    int cc = 0;

    while ((bytes = read(source, a_char, 1)) > 0)
    {   
        if ((a_char[0] == '\n') || (a_char[0] == ' '))
        {
            
            //leggo una linea, la parso, aggiungo i dati a quelli che gia' a avevo

            // linea completata, pronta per essere analizzata
            char *file;
            int char_int;
            unsigned long occurrences;

            if (file_analysis_parse_line(a_line, &file, &char_int, &occurrences))
            {
                updateList(fileList,file);

                // aggiornamento occorrenze
                count[char_int] += occurrences;
                cc += occurrences;

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
    
    //calcolo totali
    unsigned long totnp = 0;
    for (i = 0; i < 32; i++)
    {
        totnp += count[i];
    }
    totnp += count[126];
    unsigned long totM = 0;
    for (i = 65; i < 91; i++)
    {
        totM += count[i];
    }
    unsigned long totmin = 0;
    for (i = 97; i < 123; i++)
    {
        totmin += count[i];
    }
    unsigned long totnum = 0;
    for (i = 48; i < 58; i++)
    {
        totnum += count[i];
    }
    unsigned long totpunt = 0;
    for (i = 33; i < 48; i++)
    {
        totpunt += count[i];
    }
    fflush(stdout);
    for (i = 58; i < 65; i++)
    {
        totpunt += count[i];
    }
    for (i = 91; i < 97; i++)
    {
        totpunt += count[i];
    }
    for (i = 123; i < 127; i++)
    {
        totpunt += count[i];
    }
    unsigned long totprint = totM + totmin + totnum + totpunt + count[' '];
    
    
    unsigned long tot = totprint;
    if (strcmp(argv[2], "allchars") == 0)
    {
        
        tot += totnp;
    }

    for (i = 3; i < argc; i++)
    {

        if (strcmp(argv[i], "-ls") == 0) // ls = lista file
        {
            printFileList(fileList);
            printf("\nCaratteri totali rilevati: %ld\n", tot);
        }

        if (strcmp(argv[i], "-sp") == 0) //stampa gli spazi
        {
            printf("Spazi: %ld (%.2f%c)\n", count[32], ((float)count[32] / tot) * 100, '%');
        }

        if (strcmp(argv[i], "-np") == 0) // caratteri non stampabili
        {
            printf("Caratteri non stampabili: %ld (%.2f%c)\n", totnp, ((float)totnp / (totprint + totnp)) * 100, '%');
        }

        if (strcmp(argv[i], "-p") == 0) // caratteri stampabili
        {
            printf("Caratteri stampabili: %ld (%.2f%c)\n", totprint, ((float)totprint / (totprint + totnp)) * 100, '%');
        }

        if (strcmp(argv[i], "-lett") == 0) // caratteri stampabili
        {
            printf("Lettere: %ld (%.2f%c)\n", totM + totmin, ((float)(totM + totmin) / tot) * 100, '%');
        }

        if (strcmp(argv[i], "-punt") == 0) // punteggiatura
        {
            printPunt(count, totpunt, tot, false);
        }

        if (strcmp(argv[i], "-allpunt") == 0) // punteggiatura
        {
            printPunt(count, totpunt, tot, true);
        }

        if (strcmp(argv[i], "-num") == 0) // punteggiatura
        {
            printNum(count, totnum, tot, false);
        }

        if (strcmp(argv[i], "-allnum") == 0) // punteggiatura
        {
            printNum(count, totnum, tot, true);
        }

        if (strcmp(argv[i], "-M") == 0) // punteggiatura
        {
            printMaiusc(count, totM, tot, false);
        }

        if (strcmp(argv[i], "-allM") == 0) // punteggiatura
        {
            printMaiusc(count, totM, tot, true);
        }

        if (strcmp(argv[i], "-m") == 0) // punteggiatura
        {
            printMinusc(count, totmin, tot, false);
        }

        if (strcmp(argv[i], "-allm") == 0) // punteggiatura
        {
            printMinusc(count, totmin, tot, true);
        }

        if (strcmp(argv[i], "-allch") == 0) // punteggiatura
        {
            printAll(count, (totnp + totprint));
        }

        //aggregato 1: resoconto generale
        if (strcmp(argv[i], "-1") == 0) // punteggiatura
        {
            printf("Caratteri stampabili: %ld (%.2f%c)\n", totprint, ((float)totprint / (totprint + totnp)) * 100, '%');
            printf("Caratteri non stampabili: %ld (%.2f%c)\n", totnp, ((float)totnp / (totprint + totnp)) * 100, '%');
            printf("Lettere: %ld (%.2f%c)\n", totM + totmin, ((float)(totM + totmin) / tot) * 100, '%');
            printf("Spazi: %ld (%.2f%c)\n", count[32], ((float)count[32] / tot) * 100, '%');
            printNum(count, totnum, tot, false);
            printPunt(count, totpunt, tot, false);
        }

        //aggregato 2: caratteri stampabili
        if (strcmp(argv[i], "-2") == 0) // punteggiatura
        {
            printf("Caratteri stampabili: %ld (%.2f%c)\n", totprint, ((float)totprint / (totprint + totnp)) * 100, '%');
            printf("Lettere: %ld (%.2f%c)\n", totM + totmin, ((float)(totM + totmin) / tot) * 100, '%');
            printMaiusc(count, totM, tot, false);
            printMinusc(count, totmin, tot, false);
            printf("Spazi: %ld (%.2f%c)\n", count[32], ((float)count[32] / tot) * 100, '%');
            printNum(count, totnum, tot, false);
            printPunt(count, totpunt, tot, false);
        }

        //aggregato 3: lettere
        if (strcmp(argv[i], "-3") == 0) // punteggiatura
        {
            printf("Caratteri stampabili: %ld (%.2f%c)\n", totprint, ((float)totprint / (totprint + totnp)) * 100, '%');
            printf("Lettere: %ld (%.2f%c)\n", totM + totmin, ((float)(totM + totmin) / tot) * 100, '%');
            printMaiusc(count, totM, tot, true);
            printMinusc(count, totmin, tot, true);
        }

        //aggregato 4: numeri, spazi, punteggiatura
        if (strcmp(argv[i], "-4") == 0) // punteggiatura
        {
            printf("Caratteri stampabili: %ld (%.2f%c)\n", totprint, ((float)totprint / (totprint + totnp)) * 100, '%');
            printf("Spazi: %ld (%.2f%c)\n", count[32], ((float)count[32] / tot) * 100, '%');
            printNum(count, totnum, tot, true);
            printPunt(count, totpunt, tot, true);
        }

    }

    //printf("%d occorrenze di ogni carattere lette :)\n", cc);

    list_delete(fileList);

    remove(fifo_path);
    close(source);
}

//cerca una stringa nella lista, se non è presente la appende in fondo
void updateList(struct list *l, char *fileName)
{
    struct list_iterator *iter = list_iterator_new(l);
    char *curElement;
    bool done = false;
    curElement = (char *)list_iterator_next(iter);
    while ((curElement != NULL) && !done)
    {
        if (strcmp(curElement, fileName) == 0)
        {
            done = true;
        }
        else
        {
            curElement = (char *)list_iterator_next(iter);
        }
    }

    if (!done)
    {
        curElement = (char *)malloc(sizeof(char) * (strlen(fileName) + 1));
        strcpy(curElement, fileName);
        list_push(l, curElement);
    }

    list_iterator_delete(iter);
}

void printFileList(struct list *l)
{
    struct list_iterator *iter = list_iterator_new(l);
    char *curElement;
    curElement = (char *)list_iterator_next(iter);
    printf("\nFile List:\n");
    while (curElement != NULL)
    {
        printf("%s\n", curElement);
        curElement = (char *)list_iterator_next(iter);
    }
    list_iterator_delete(iter);
}

void printPunt(unsigned long *count, unsigned long totpunt, unsigned long tot, bool all)
{
    if(all)printf("\n");
    printf("Punteggiatura: %ld (%.2f%c)\n", totpunt, ((float)totpunt / tot) * 100, '%');
    if (all)
    {
        int i;
        for (i = 33; i < 48; i++)
        {
            if (count[i] > 0)
            {
                printf("caratteri 0x%02X (%c): %ld (%.2f%c)\n", i, i, count[i], ((float)count[i] / tot) * 100, '%');
            }
        }
        for (i = 58; i < 65; i++)
        {
            if (count[i] > 0)
            {
                printf("caratteri 0x%02X (%c): %ld (%.2f%c)\n", i, i, count[i], ((float)count[i] / tot) * 100, '%');
            }
        }
        for (i = 91; i < 97; i++)
        {
            if (count[i] > 0)
            {
                printf("caratteri 0x%02X (%c): %ld (%.2f%c)\n", i, i, count[i], ((float)count[i] / tot) * 100, '%');
            }
        }
        for (i = 123; i < 127; i++)
        {
            if (count[i] > 0)
            {
                printf("caratteri 0x%02X (%c): %ld (%.2f%c)\n", i, i, count[i], ((float)count[i] / tot) * 100, '%');
            }
        }
    }
}

void printNum(unsigned long *count, unsigned long totnum, unsigned long tot, bool all)
{
    if(all)printf("\n");
    printf("Numeri: %ld (%.2f%c)\n", totnum, ((float)totnum / tot) * 100, '%');
    if (all)
    {
        int i;
        for (i = 48; i < 58; i++)
        {
            if (count[i] > 0)
            {
                printf("caratteri 0x%02X (%c): %ld (%.2f%c)\n", i, i, count[i], ((float)count[i] / tot) * 100, '%');
            }
        }
    }
}

void printMaiusc(unsigned long *count, unsigned long totM, unsigned long tot, bool all)
{
    if(all)printf("\n");
    printf("Lettere Maiuscole: %ld (%.2f%c)\n", totM, ((float)totM / tot) * 100, '%');
    if (all)
    {
        int i;
        for (i = 65; i < 91; i++)
        {
            if (count[i] > 0)
            {
                printf("caratteri 0x%02X (%c): %ld (%.2f%c)\n", i, i, count[i], ((float)count[i] / tot) * 100, '%');
            }
        }
    }
}

void printMinusc(unsigned long *count, unsigned long totmin, unsigned long tot, bool all)
{
    if(all)printf("\n");
    printf("Lettere Minuscole: %ld (%.2f%c)\n", totmin, ((float)totmin / tot) * 100, '%');
    if (all)
    {
        int i;
        for (i = 97; i < 123; i++)
        {
            if (count[i] > 0)
            {
                printf("caratteri 0x%02X (%c): %ld (%.2f%c)\n", i, i, count[i], ((float)count[i] / tot) * 100, '%');
            }
        }
    }
}

void printAll(unsigned long *count, unsigned long tot)
{
    int i;
    char str[8];
    printf("\n Dati di ogni carattere ASCII:\n");
    for (i = 0; i < 128; i++)
    {
        if (count[i] > 0)
        {
            to_string(i, str);
            printf("caratteri 0x%02X (%s): %ld (%.2f%c)\n", i, str, count[i], ((float)count[i] / tot) * 100, '%');
        }
    }
}

void to_string(char c, char *s)
{
    char *non_printable[33] = {
        "NULL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL", "BS", "HT",
        "LF", "VT", "FF", "CR", "SO", "SI", "DLE", "DC1", "DC2", "DC3", "DC4",
        "NAK", "SYN", "ETB", "CAN", "EM", "SUB", "ESC", "FS", "GS", "RS", "US"};

    memset(s, 0, 8);
    if (32 <= c && c <= 126)
    {
        s[0] = s[2] = '\'';
        s[1] = c;
    }
    else if (c == 127)
    {
        strcat(s, "DEL");
    }
    else
    {
        strcat(s, non_printable[c]);
    }
}

int numOfDigits(int n)
{
    int c = 0;
    while (n > 0)
    {
        n = (int)n / 10;
        c++;
    }
    return c;
}
