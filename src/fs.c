#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "list.h"

int is_directory(char *path)
{
    struct stat stat_;

    if (access(path, F_OK) == -1)
    {
        return -1;
    }
    if (stat(path, &stat_) != 0)
    {
        return 0;
    }
    return S_ISDIR(stat_.st_mode);
}

struct list *ls(char *path)
{
    char *cmd = (char *)malloc(sizeof(char) * (strlen(path) + 40));
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