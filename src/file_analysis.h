#ifndef _FILE_ANALYSIS_H_
#define _FILE_ANALYSIS_H_

struct file_analysis
{
    char *file;
    unsigned long analysis[128];
};

struct file_analysis *file_analysis_new();
void file_analysis_delete(struct file_analysis *file_analysis);

#endif