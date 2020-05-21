#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "list.h"

struct list *list_new()
{
    struct list *new_list = (struct list *)malloc(sizeof(struct list));
    new_list->first = new_list->last = NULL;
    new_list->lenght = 0;
    return new_list;
}

void list_delete(struct list *list)
{
    struct list_item *item = list->first;
    struct list_item *next;
    while (item != NULL)
    {
        next = item->next;
        free(item);
        item = next;
    }
}

void list_push(struct list *list, void *data)
{
    struct list_item *new_item = (struct list_item *)malloc(sizeof(struct list_item));
    new_item->data = data;
    new_item->next = NULL;

    list->lenght++;

    if (list->first == NULL)
    {
        // lista vuota
        list->first = list->last = new_item;
    }
    else
    {
        list->last->next = new_item;
        list->last = new_item;
    }
}

struct list_iterator *list_iterator_new(struct list *list)
{
    struct list_iterator *new_iter = (struct list_iterator *)malloc(sizeof(struct list_iterator));
    new_iter->current = list->first;
    new_iter->unused = true;

    return new_iter;
}

void list_iterator_delete(struct list_iterator *iterator)
{
    free(iterator);
}

void *list_iterator_next(struct list_iterator *iterator)
{
    if (iterator->unused) {
        iterator->unused = false;
    }

    if (iterator->current == NULL) {
        return NULL;
    }

    void *ret = iterator->current->data;
    iterator->current = iterator->current->next;

    return ret;
}