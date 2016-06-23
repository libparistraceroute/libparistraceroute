#include "config.h"

#include <stdlib.h> // malloc, free

#include "list.h"

list_cell_t * list_cell_create(void * element)
{
    list_cell_t * list_cell;
   
    if ((list_cell = malloc(sizeof(list_cell_t)))) {
        list_cell->element = element;
        list_cell->next = NULL;
    }

    return list_cell;
}

void list_cell_free(list_cell_t * list_cell, void (*element_free)(void * element))
{
    if (element_free) element_free(list_cell->element);
    free(list_cell);
}

list_t * list_create() {
    return calloc(1, sizeof(list_t));
}

void list_free(list_t * list, void (*element_free)(void * element))
{
    list_cell_t * list_cell,
                * prev_cell = NULL;

    if (list) {
        for (list_cell = list->head; list_cell; list_cell = list_cell->next) {
            // Free the previous cell (if any)
            if (prev_cell) {
                list_cell_free(prev_cell, element_free);
            }

            // Update the pointer to the previous cell
            prev_cell = list_cell;
        }

        if (prev_cell) {
            list_cell_free(prev_cell, element_free);
        }

        free(list);
    }
}

bool list_push_element(list_t * list, void * element)
{
    list_cell_t * list_cell;
    bool          ret = true;
   
    if ((list_cell = list_cell_create(element))) {
        if (!list->head) {
            list->head = list_cell;
        } else {
            list->tail->next = list_cell;
        }
        list->tail = list_cell;
    } else ret = false;

    return ret;
}

void * list_pop_element(list_t * list, void (*element_free)(void * element))
{
    list_cell_t * list_cell;
    void        * element = NULL;

    if ((list_cell = list->head)) {
        list->head = list_cell->next;
        if (!list->head) {
            list->tail = NULL;
        }
        element = list_cell->element;
        list_cell_free(list_cell, element_free);
    }

    return element;
}
