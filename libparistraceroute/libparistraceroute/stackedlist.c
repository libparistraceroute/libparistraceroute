#include "stackedlist.h"

stackedlist_element_t *stackedlist_element_create(void)
{
    stackedlist_element_t *elt;
    
    elt = (stackedlist_element_t*)malloc(sizeof(stackedlist_element_t));
    elt->element = NULL;
    elt->next = NULL;

    return elt;
}


stackedlist_level_t * stackedlist_level_create(void)
{
    stackedlist_level_t *level;

    level = malloc(sizeof(stackedlist_level_t));

    level->head = NULL;
    level->next = NULL;

    return level;
}

void stackedlist_level_free(stackedlist_level_t *level)
{
    // TODO free all list
    
    free(level);
    level = NULL;
}

void stackedlist_level_push(stackedlist_t *list)
{
    stackedlist_level_t *level;

    level = stackedlist_level_create();
    level->next = list->top_level;
    level->head = NULL;

    list->top_level = level;
}

void stackedlist_level_pop(stackedlist_t *list)
{
    stackedlist_level_t *level;

    level = list->top_level;
    list->top_level = level->next;
    stackedlist_level_free(level);
}

void stackedlist_level_add_element(stackedlist_level_t *level, void *element)
{
    stackedlist_element_t *elt;

    elt = stackedlist_element_create();
    elt->element = element;
    elt->next = level->head;
    level->head = elt;
}

stackedlist_t *stackedlist_create(void)
{
    stackedlist_t *list;
    
    list = malloc(sizeof(stackedlist_t));
    stackedlist_level_push(list);

    return list;
}

void stackedlist_free(stackedlist_t *list)
{
    // TODO free all levels

    free(list);
    list = NULL;
}

void stackedlist_add_element(stackedlist_t *list, void *element)
{
    stackedlist_level_add_element(list->top_level, element);
}

void stackedlist_iter_elements(stackedlist_t *list, void *data, void (*callback)(void *element, void *data))
{
    stackedlist_level_t *level;
    stackedlist_element_t *element;


    for (level = list->top_level; level; level++) {
        for (element = level->head; element; element++) {
            callback(element->element, data);
        }
    }
}
