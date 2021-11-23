#include "list.h"

#include <stdlib.h>

#include "log.h"

struct list_entry
{
    any_t data;
    struct list_entry *next;
};

struct list_it
{
    struct list_entry *current;
};

struct list_entry *list_entry_new(any_t item)
{
    struct list_entry *entry = malloc(sizeof(struct list_entry));
    entry->data = item;
    entry->next = NULL;
    return entry;
};

list_t list_init()
{
    return (list_t)list_entry_new(NULL);
};

void list_free(list_t list, bool free_data)
{
    struct list_entry *current = list->next;
    while (current != NULL)
    {
        if (free_data && current->data)
            free(current->data);

        struct list_entry *next = current->next;
        free(current);
        current = next;
    }

    free(list);
}

unsigned int list_len(list_t list)
{
    struct list_entry *current = list->next;
    unsigned int len = 0;

    while (current != NULL)
    {
        len++;
        current = current->next;
    }
    return len;
};

enum list_status list_add(list_t list, any_t item)
{
    struct list_entry *current = list;

    while (current->next != NULL)
    {
        current = current->next;
    }

    current->next = list_entry_new(item);
    return LIST_OK;
};

enum list_status list_remove(list_t list, any_t item)
{
    struct list_entry *last = list;
    struct list_entry *current = last->next;

    while (current != NULL)
    {
        if (current->data == item)
        {
            last->next = current->next;
            free(current);
            return LIST_OK;
        }

        last = current;
        current = current->next;
    }

    return LIST_NOT_FOUND;
};

list_it_t list_it_new(list_t list)
{
    struct list_entry *current = list->next;
    struct list_it *it = malloc(sizeof(struct list_it));
    it->current = current;
    return it;
};

enum list_status list_it_next(list_it_t it, any_t *item)
{
    if (it->current == NULL)
    {
        if (*item)
            *item = NULL;

        return LIST_NOT_FOUND;
    }

    *item = it->current->data;
    it->current = it->current->next;
    return LIST_OK;
};

void list_it_free(list_it_t it)
{
    free(it);
};