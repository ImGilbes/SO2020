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