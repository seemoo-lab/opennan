#ifndef NAN_LIST_H_
#define NAN_LIST_H_

#include <stdbool.h>

/**
 * Pointer for arbitrary data
 */
typedef void *any_t;

/**
 * Opaque list implementation
 */
struct list_entry;
typedef struct list_entry *list_t;

/**
 * Opaque iterator implementation
 */
struct list_it;
typedef struct list_it *list_it_t;

enum list_status
{
    LIST_OK = 0,
    LIST_OMEM = -1,
    LIST_NOT_FOUND = -2,
};

typedef bool (*list_item_matches)(any_t item, any_t value);

list_t list_init();

void list_free(list_t list, bool free_data);

unsigned int list_len(list_t list);

enum list_status list_add(list_t list, any_t item);

enum list_status list_remove(list_t list, any_t item);

list_it_t list_it_new(list_t list);

enum list_status list_it_next(list_it_t it, any_t *item);

void list_it_free(list_it_t it);

#define LIST_FOR_EACH(list, item_ptr, func)                     \
    do                                                          \
    {                                                           \
        list_it_t it = list_it_new(list);                       \
        while (list_it_next(it, (any_t *)&item_ptr) == LIST_OK) \
        {                                                       \
            func;                                               \
        }                                                       \
        list_it_free(it);                                       \
    } while (0);

#define LIST_FILTER_FOR_EACH(list, item_ptr, condition, func)   \
    do                                                          \
    {                                                           \
        list_it_t it = list_it_new(list);                       \
        while (list_it_next(it, (any_t *)&item_ptr) == LIST_OK) \
        {                                                       \
            if (condition)                                      \
            {                                                   \
                func;                                           \
            }                                                   \
        }                                                       \
        list_it_free(it);                                       \
    } while (0);

#define LIST_FIND(list, item_ptr, condition) \
    do                                       \
    {                                        \
        int found = 0;                       \
        LIST_FOR_EACH(                       \
            list, item_ptr,                  \
            if (condition) {                 \
                found = 1;                   \
                break;                       \
            })                               \
        if (found == 0)                      \
            item_ptr = NULL;                 \
    } while (0);

#define LIST_REMOVE(list, item_ptr, condition)  \
    do                                          \
    {                                           \
        LIST_FIND(list, item_ptr, condition)    \
        if (item_ptr != NULL)                   \
            list_remove(list, (any_t)item_ptr); \
    } while (0);

#endif // NAN_LIST_H_