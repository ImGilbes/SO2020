#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "list.h"

// -1 il file non esiste, 0 non e' una directory, 1 e' una directory
int is_directory(char *path)
{
    struct stat stat_;

    if (access(path, F_OK) == -1)
    {
        return -1; // non esiste
    }
    stat(path, &stat_);
    return S_ISDIR(stat_.st_mode);
}

// -1 il file non esiste, 0 non e' un file ASCII, 1 e' un file ASCII
int is_ascii_file(char *path) {
    char *cmd = (char *)malloc(sizeof(char) * (strlen(path) + 6));
    strcpy(cmd, "file ");
    strcat(cmd, path);

    if (access(path, F_OK) == -1)
    {
        return -1; // non esiste
    }

    FILE *fp;   //file descriptor pipe
    char *buff; //buffer di lettura dalla pipe

    fp = popen(cmd, "r"); //apre l'end della pipe su questo processo in lettura
    if (fp == NULL)
    {
        printf("Errore apertura pipe\n");
    }

    fscanf(fp, "%m[^\n]s", &buff);
    int ret = strstr(buff, "ASCII text") ? 1 : 0;

    free(cmd);
    free(buff);
    pclose(fp);
    return ret;
}

struct list *ls(char *path)
{
    char *cmd = (char *)malloc(sizeof(char) * (strlen(path) + 4));
    strcpy(cmd, "ls ");
    strcat(cmd, path);

    FILE *fp;   //file descriptor pipe
    char *buff; //buffer di lettura dalla pipe
    struct list *flist;
    flist = list_new();

    fp = popen(cmd, "r"); //apre l'end della pipe su questo processo in lettura
    if (fp == NULL)
    {
        printf("Errore apertura pipe\n");
    }

    while ((fscanf(fp, "%ms", &buff) != EOF))
    {
        //printf("Il nome letto: %s\n", buff);
        char *file_path = (char *)malloc(sizeof(char) * (strlen(path) + 1 + strlen(buff) + 1));
        strcpy(file_path, path);
        strcat(file_path, "/");
        strcat(file_path, buff);
        list_push(flist, file_path);
    }

    free(cmd);
    free(buff);
    pclose(fp);
    return flist;
}