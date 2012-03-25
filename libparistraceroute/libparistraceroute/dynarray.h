#ifndef DYNARRAY_H
#define DYNARRAY_H

typedef struct {
    void **elements;
    unsigned int size;
    unsigned int max_size;
} dynarray_t;

dynarray_t* dynarray_create(void);
void dynarray_free(dynarray_t *dynarray, void (*element_free)(void *element));
void dynarray_push_element(dynarray_t *dynarray, void *element);
void dynarray_clear(dynarray_t *dynarray, void (*element_free)(void *element));
unsigned int dynarray_get_size(dynarray_t *dynarray);
void **dynarray_get_elements(dynarray_t *dynarray);

#endif
