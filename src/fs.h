#ifndef _FS_H_
#define _FS_H_

#include "list.h"

int is_directory(char *file);
int is_executable(char *path);
struct list *ls(char *path);
int is_ascii_file(char *path);

#endif