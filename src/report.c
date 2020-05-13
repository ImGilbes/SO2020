#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "file_analysis.h"
#include "list.h"

#define delim ":"

char* to_string(char c, char *s) {
    char *non_printable[33] = {
        "NULL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL", "BS", "HT",
        "LF", "VT", "FF", "CR", "SO", "SI", "DLE", "DC1", "DC2", "DC3", "DC4",
        "NAK", "SYN", "ETB", "CAN", "EM", "SUB", "ESC", "FS", "GS", "RS", "US"
    };

    memset(s, 0, 8);
    if (32 <= c && c <= 126) {
        s[0] = s[2] = '\'';
        s[1] = c;
    } else if (c == 127) {
        strcat(s, "DEL");
    } else {
        strcat(s, non_printable[c]);
    }
}

struct file_analysis *fileStructPointer(struct list *analysisList, char *fileName);

int main(int argc, char **argv){
    bool doneFlag = false;
    char *readBuff;
    struct list *fileList = list_new(); //lista contenente tutti i file_analysis* usati
    struct file_analysis *curFile; //puntatore alla struttura file da aggiornare e da cui prendere dati
    struct list_iterator *iter;
    int* totChars; //non so di quanti file raccolgo i dati => array dinamico
    int i,k, localsum, charID;
    unsigned long aggregated[128];
    unsigned long aggrsum = 0;

    while(!doneFlag){ //ricevo input finchè non riceve comando di terminazione
        scanf("%ms",&readBuff);
        if(strcmp(readBuff,"done") != 0){ //done è il comando di fine input
            //leggo una linea, la parso, aggiungo i dati a quelli che già a avevo
            curFile = fileStructPointer(fileList, strtok(readBuff, delim)); //prendo la file_analysis associata con il file anme di questa riga
            charID = atoi(strtok(NULL,delim));
            curFile->analysis[charID] = atoi(strtok(NULL,delim));//aggiungo i dati sul carattere nella struttura apposita*/
        }
        else{
            doneFlag = true;
        }
    }

    //CICLO 1 : calcolo totale caretteri per ogni file e totale caratteri aggregato
    totChars = (int*)malloc(sizeof(int) * (fileList->lenght));
    iter = list_iterator_new(fileList);
    k=0;
    while ((curFile = (struct file_analysis *)list_iterator_next(iter))){
        localsum = 0;
        for(i = 0; i < 128; i++ ){
            localsum += curFile->analysis[i];
        }
        totChars[k] = localsum;
        aggrsum += localsum;
        k++;
    }

    //CILO 2: init aggregated sums for charactes
    for(i = 0; i < 128; i++){
        aggregated[i] = 0;
    }

    //CICLO 3: Per ogni file_analysis struct creata, printo le percentuali di
    //presenza dei caratteri con il format specificato in README
    iter = list_iterator_new(fileList);
    k=0;
    char char_as_string[8]; // per poter visualizzare i caratteri non stampabili con il loro acronimo
    while ((curFile = (struct file_analysis *)list_iterator_next(iter))){
        printf("%s \n", curFile->file);
        for(i = 0; i < 128; i++ ){
            to_string(i, char_as_string);

            if(curFile->analysis[i] > 0){ //stampo i dati su un caratteri sse è stato trovato almeno una volta
                printf("caratteri 0x%02X (%s): %ld (%.2f%c) \n", i, char_as_string, curFile->analysis[i], ((float) curFile->analysis[i] / totChars[k]),'%');
                aggregated[i] += curFile->analysis[i];
            }
        }
        k++;
    }

    printf("\nAggregated data\n");
    for( i = 0; i < 128; i++){
        if(aggregated[i] > 0){
            to_string(i, char_as_string);
            printf("caratteri 0x%02X (%s): %ld (%.2f%c)\n", i, char_as_string, aggregated[i], (float)aggregated[i]/aggrsum,'%');
        }
    }

    return 0;
}

//returns the pointer to the file_analysis structure with fileName as file name
//if such structure does not exist, it creates a new struct at the end of the list and returns
//a pointer to that struct
struct file_analysis *fileStructPointer(struct list *analysisList, char *fileName){
    struct list_iterator *iter = list_iterator_new(analysisList);
    struct file_analysis *curElement;

    while ((curElement = (struct file_analysis *)list_iterator_next(iter))){
        if(strcmp(curElement->file, fileName) == 0){
            return curElement;
        }
    }

    //creo una nuova struttura e la pusho in lista
    curElement = file_analysis_new();
    curElement->file = (char *)malloc(sizeof(char) * strlen(fileName)); //forse serve strlen+1 qui
    strcpy(curElement->file, fileName);
    list_push(analysisList, curElement);
    return curElement;
}
