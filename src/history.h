#ifndef _HISTORY_H_
#define _HISTORY_H_

#include <time.h>

struct history {
    struct list *data;
    struct list *resources;
    time_t timestamp;
};

struct history* history_new();

#endif