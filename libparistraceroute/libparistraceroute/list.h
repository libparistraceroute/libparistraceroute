#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

typedef struct list_elt_s {
    void *element;
    struct list_elt_s *next;
} list_elt_t;

typedef struct {
        list_elt_t *head;
        list_elt_t *tail;
} list_t;

list_elt_t *list_elt_create(void *element);
void list_elt_free(list_elt_t *qe, void (*element_free)(void *element));

list_t *list_create(void);
void list_free(list_t *list, void (*element_free)(void *element));
void list_push_element(list_t *list, void *element);
void *list_pop_element(list_t *list);

#endif
