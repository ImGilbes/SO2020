#ifndef _FS_H_
#define _FS_H_

#include "list.h"

int is_directory(char *file);
struct list *ls(char *path);

#endif