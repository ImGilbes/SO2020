#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "file_analysis.h"
#include "utilities.h"

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

bool file_analysis_parse_line(char *line, char **file, int *char_base10, unsigned long *occurrences)
{
    if (line == NULL || strlen(line) == 0)
        return false;

    char *rest;
    char *tmp = strtok_r(line, ":", &rest);
    char *file_ = tmp ? strdup(tmp) : NULL;
    // printf("file: %s\n", file_);

    tmp = strtok_r(NULL, ":", &rest);
    char *char_ = tmp ? strdup(tmp) : NULL;
    // printf("char: %s\n", char_);

    tmp = strtok_r(NULL, ":", &rest);
    char *occurrences_ = tmp ? strdup(tmp) : NULL;
    // printf("occ: %s\n", occurrences_);

    // sono presenti almeno i 3 campi
    if (!(file_ && char_ && occurrences_))
        return false;

    // non ci sono altri campi
    char *extra_colon = strtok_r(NULL, ":", &rest);
    if (extra_colon)
        return false;
    
    // char e ocurrences sono di sole cifre
    if (!is_positive_number(char_) || !is_positive_number(occurrences_))
        return false;
    
    // tutti i test sono passati
    *file = file_;
    *char_base10 = atoi(char_);
    *occurrences = atoi(occurrences_);

    free(char_);
    free(occurrences_);

    return 1;
}