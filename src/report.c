#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "file_analysis.h"
#include "list.h"

#define delim ":"

struct file_analysis *fileStructPointer(struct list *analysisList, char *fileName);

int main(int argc, char **argv){
    bool doneFlag = false;
    char buff[100]; //100: limitazione superiore per la lunghezza di una riga
    struct list *fileList = list_new(); //lista contenente tutti i file_analysis* usati
    struct file_analysis *curFile; //puntatore alla struttura file da aggiornare e da cui prendere dati
    int* totChars;
    int i,k, localsum, charID;
    struct list_iterator *iter;
    unsigned long aggregated[128];
    unsigned long aggrsum = 0;

    while(!doneFlag){ //ricevo input finchè non riceve comando di terminazione
        scanf("%s",buff);
        if(strcmp(buff,"done") != 0){ //done è il comando di fine input
            //leggo una linea, la parso, aggiungo i dati a quelli che già a avevo
            curFile = fileStructPointer(fileList, strtok(buff, delim)); //prendo la file_analysis associata con il file anme di questa riga
            charID = atoi(strtok(NULL,delim));
            curFile->analysis[charID] = atoi(strtok(NULL,delim));//aggiungo i dati sul carattere nella struttura apposita*/
            //Passo alla prossima linea
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
            aggregated[i] = 0; //l'ho messo qui per non dover fare un'altro ciclo solo per inizializzarlo
        }
        totChars[k] = localsum;
        aggrsum += localsum;
        k++;
    }

    //CILO 2: init aggregated sums for charactes
    for(i = 0; i < 128; i++){
        aggregated[i] = 0;
    }

    //Per ogni file_analysis struct creata, pusho in output le percentuali di presenza dei caratteri
    //con il format specificato in README
    iter = list_iterator_new(fileList);
    k=0;
    while ((curFile = (struct file_analysis *)list_iterator_next(iter))){
        printf("%s \n", curFile->file);
        for(i = 0; i < 128; i++ ){
            if(curFile->analysis[i] > 0){ //stampo i dati su un caratteri sse è stato trovato almeno una volta
                printf("caratteri %c: %ld (%f) \n", i, curFile->analysis[i], ((float) curFile->analysis[i] / totChars[k]));
                aggregated[i] += curFile->analysis[i];
            }
        }
        k++;
    }

    printf("\nAggregated data \n");
    for( i = 0; i < 128; i++){
        if(aggregated[i] > 0){
            printf("caratteri %c: %ld (%f)\n", i, aggregated[i], (float)aggregated[i]/aggrsum);
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
