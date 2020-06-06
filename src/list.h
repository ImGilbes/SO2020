#ifndef _LIST_H_
#define _LIST_H_

#include "bool.h"

struct list_item
{
    void *data;
    struct list_item *next;
};

struct list
{
    struct list_item *first;
    struct list_item *last;
    unsigned int lenght;
};

struct list_iterator
{
    struct list_item *current;
    bool unused;
};

struct list *list_new();
void list_delete(struct list *list);
bool list_delete_file_of_file_analysis(struct list *list, char *item);
void list_push(struct list *list, void *data);
struct list_iterator *list_iterator_new(struct list *list);
void list_iterator_delete(struct list_iterator *iterator);
void *list_iterator_next(struct list_iterator *iterator);
bool list_is_empty(struct list *list);
void *list_pop(struct list *list);

#endif