#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "file_analysis.h"
#include "list.h"
#include "bool.h"

bool list_is_empty(struct list *list)
{
    return (list->first == NULL && list->last == NULL);
}

bool list_delete_file_of_file_analysis(struct list *list, char *item)
{
    struct list_item *tmp, *prev;
	
    //printf("Sto rimuovendo %s \n", item);
    if (list_is_empty(list)) 
    {
        return false;
    }

    tmp = list->first;

    if (strcmp((char *)((struct file_analysis *)(tmp->data))->file, item) == 0)
    {
	//printf("Son in A\n");
        if (list->first == list->last)
        {
            list->first = list->last = NULL;
        }
        else
        {
	    //printf("Son in B\n");
            list->first = tmp->next;
        }
        
        file_analysis_delete((struct file_analysis *)tmp->data);
        free(tmp);
        list->lenght--;
        return true;
    }
    else
    {
        while (tmp != NULL && strcmp((char *)((struct file_analysis *)(tmp->data))->file, item) != 0)
        {
            prev = tmp;
            tmp = tmp->next;
        }

        if (tmp == NULL)
        {
            return false;
        }
        else
        {
            if (tmp == list->last)
            {
                list->last = prev;
            }
            
            prev->next = tmp->next;

            file_analysis_delete((struct file_analysis *)tmp->data);
            free(tmp);
	        list->lenght--;
            return true;
        }
        
    }
}

void *list_pop(struct list *list)
{
    void *ret = NULL;

    if (list_is_empty(list))
    {
        return ret;

    } 
    else
    {
        ret = list->last->data;

        if (list->first == list->last)
        {
            free(list->last);
            list->first = list->last = NULL;
        }
        else
        {
            struct list_item *pred = list->first;
            while (pred->next != list->last)
            {
                pred = pred->next;
            }
            free(list->last);
            list->last = pred;
            pred->next = NULL;

        }  

        list->lenght--;
        return ret;
    }    
}

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
    //free(list);
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
    if (iterator->unused)
    {
        iterator->unused = false;
    }

    if (iterator->current == NULL)
    {
        return NULL;
    }

    void *ret = iterator->current->data;
    iterator->current = iterator->current->next;

    return ret;
}
