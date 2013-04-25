#include <stdlib.h>
#include "list.h"

list_elt_t *list_elt_create(void *element)
{
    list_elt_t* qe;

    qe = (list_elt_t*)malloc(sizeof(list_elt_t));
    qe->element = element;
    qe->next = NULL;

    return qe;
}

void list_elt_free(list_elt_t *qe, void (*element_free)(void *element))
{
    if (element_free)
        element_free(qe->element);
    free(qe);
}

list_t *list_create(void)
{
    list_t *list;

    list = (list_t*)malloc(sizeof(list_t));
    list->head = NULL;
    list->tail = NULL;
    return list;
}

void list_free(list_t *list, void (*element_free)(void *element))
{
    /* free all list elements */
    if (element_free) {
        list_elt_t *qe, *qe_to_free = NULL;
        for(qe = list->head; qe; qe = qe->next)
            if (qe_to_free)
                list_elt_free(qe_to_free, element_free);
            qe_to_free = qe;
        if (qe_to_free)
            list_elt_free(qe_to_free, element_free);
    }
    free(list);
}

void list_push_element(list_t *list, void *element)
{
    list_elt_t *qe = list_elt_create(element);

    if (!list->head)
        list->head = qe;
    else
        list->tail->next = qe;
    list->tail = qe;
}

void *list_pop_element(list_t *list)
{
    list_elt_t *qe;
    void *element;

    qe = list->head;
    if (!qe)
        return NULL;

    list->head = qe->next;
    if (!list->head)
        list->tail = NULL;

    element = qe->element;
    list_elt_free(qe,false);
    return element;
}
