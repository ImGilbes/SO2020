#ifndef _FILE_ANALYSIS_H_
#define _FILE_ANALYSIS_H_

#include "bool.h"

struct file_analysis
{
    char *file;
    unsigned long analysis[128];
};

struct file_analysis *file_analysis_new();
void file_analysis_delete(struct file_analysis *file_analysis);
bool file_analysis_parse_line(char *line, char **file, int *char_base10, unsigned long *occurrences);

#endif