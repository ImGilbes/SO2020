#include <stdlib.h>
#include "history.h"

#include "list.h"

struct history* history_new() {
    struct history *new = (struct history *)malloc(sizeof(struct history));
    new->data = list_new();
    new->resources = list_new();
    new->timestamp = 0;

    return new;
}
