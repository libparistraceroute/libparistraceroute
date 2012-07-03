#ifndef STACKEDLIST
#define STACKEDLIST

#include <stdlib.h>

typedef struct stackedlist_element_s {
    void *element;
    struct stackedlist_element_s *next;
} stackedlist_element_t;

typedef struct stackedlist_level_s {
    stackedlist_element_t *head;
    struct stackedlist_level_s *next;
} stackedlist_level_t;

typedef struct {
    stackedlist_level_t *top_level;
} stackedlist_t;


stackedlist_element_t *stackedlist_element_create(void);

stackedlist_level_t * stackedlist_level_create(void);
void stackedlist_level_free(stackedlist_level_t *level);
void stackedlist_level_push(stackedlist_t *list);
void stackedlist_level_pop(stackedlist_t *list);

void stackedlist_level_add_element(stackedlist_level_t *level, void *element);

stackedlist_t *stackedlist_create(void);
void stackedlist_free(stackedlist_t *list);

void stackedlist_add_element(stackedlist_t *list, void *element);

void stackedlist_iter_elements(stackedlist_t *list, void *data, void (*callback)(void *element, void *data));

#endif
