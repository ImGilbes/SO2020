#include <stdlib.h>
#include <string.h>

#include "file_analysis.h"

struct file_analysis *file_analysis_new()
{
    struct file_analysis *new_file_analysis = (struct file_analysis *)malloc(sizeof(struct file_analysis));
    new_file_analysis->file = NULL;
    memset(new_file_analysis->analysis, 0, sizeof(new_file_analysis->analysis));

    return new_file_analysis;
}

void file_analysis_delete(struct file_analysis *file_analysis)
{
    free(file_analysis);
}

void file_analysis_parse_line(char *line, char **file, int *char_base10, int *occurrences)
{
    char *file_ends = strchr(line, ':');
    char *char_ends = strchr(file_ends + 1, ':');

    // file
    *file = (char *)malloc(sizeof(char) * (file_ends - line + 1));
    memset(*file, 0, file_ends - line + 1);
    strncpy(*file, line, file_ends - line);

    // char_base10 ed occurrences
    char *tmp = (char *)malloc(sizeof(char) * 8);
    strncpy(tmp, file_ends + 1, char_ends - file_ends - 1);
    *char_base10 = atoi(tmp);
    *occurrences = atoi(char_ends + 1);

    free(tmp);
}