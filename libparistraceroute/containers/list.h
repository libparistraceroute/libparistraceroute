#ifndef LIST_H
#define LIST_H

/**
 * \file list.h
 * \brief Header file for linked lists
 */

#include <stdbool.h>
#include <stdio.h>
#include "common.h"

/**
 * \struct list_cell_t
 * \brief Structure describing an element of a list
 */

typedef struct list_cell_s {
    void               * element; /**< Points to the element stored in this cell */
    struct list_cell_s * next;    /**< Points to the next cell in the list */
} list_cell_t;

/**
 * \struct list_t
 * \brief Structure describing a list. We assume that the list store
 *   homogeneous element, so we can store a callback to free
 *   and display each stored element.
 */

typedef struct {
    list_cell_t * head; /**< Pointer to the head of the list */
    list_cell_t * tail; /**< Pointer to the tail of the list */

    // Callbacks related to stored elements.
    void (* element_free)(void * element);
    void (* element_fprintf)(FILE * out, const void * element);
} list_t;

/**
 * \brief Create a new element ready to be added to the list.
 * \param element Pointer to the element to turn into a list element structure
 * \return List element structure created from element if successfull, NULL
 *   otherwise (and errno is set to ENOMEM).
 */

list_cell_t * list_cell_create(void * element);

/**
 * \brief Delete a list element structure.
 * \param list_cell The structure to delete.
 * \param element_free Pointer to a function to delete the element
 *    contained within the list element structure.
 */

void list_cell_free(list_cell_t * list_cell, void (*element_free) (void * element));

/**
 * \brief Create a list of elements.
 * \param element_free Callback used to free element (may be set to NULL).
 *    If NULL, the list does not release references that it contains.
 * \param element_fprintf Callback used to dump element (may be set to NULL).
 *    If NULL, the list cannot be printed using set_dump.
 */

list_t * list_create_impl(
    void   (*element_free)(void * element),
    void   (*element_fprintf)(FILE * out, const void * element)
);

#define list_create(element_free, element_fprintf) list_create_impl(\
    (ELEMENT_FREE)    element_free, \
    (ELEMENT_FPRINTF) element_fprintf \
)

/**
 * \brief Delete a list
 * \param list Pointer to the list to be deleted
 * \param element_free Pointer to a function to delete the elements contained within the list element structures that make up the list
 */

//void list_free(list_t * list, void (*element_free)(void * element));
void list_free(list_t * list);

/**
 * \brief Push an element onto the list
 * \param list Pointer to the list to push an element on to
 * \param element Pointer to the element to add to the list
 * \return true iif successfull
 */

bool list_push_element(list_t * list, void * element);

/**
 * \brief Pop an element from the list
 * \param list Pointer to the list to pop an element from
 */

void * list_pop_element(list_t * list, void (*element_free)(void * element));

/**
 * \brief Print a list into a file.
 * \param out The file descriptor of the output file.
 * \param list The list to print.
 */

void list_fprintf(FILE * out, const list_t * list);

/**
 * \brief Print a list.
 * \param list The list to print.
 */

void list_dump(const list_t * list);

#endif
