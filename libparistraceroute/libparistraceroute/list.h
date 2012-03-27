#ifndef LIST_H
#define LIST_H

/**
 * \file list.h
 * \brief Header file for linked lists
 */

#include <stdbool.h>

/**
 * \struct list_elt_t
 * \brief Structure describing an element of a list
 */
typedef struct list_elt_s {
    /** Pointer to this element */
    void *element;
    /** Pointer to the next structure in the list */
    struct list_elt_s *next;
} list_elt_t;

/**
 * \struct list_t
 * \brief Structure describing a list
 */
typedef struct {
	/** Pointer to the head of the list */
        list_elt_t *head;
	/** Pointer to the tail of the list */
        list_elt_t *tail;
} list_t;

/**
 * \brief Create a new element ready to be added to the list
 * \param element Pointer to the element to turn into a list element structure
 * \return List element structure created from element
 */
list_elt_t *list_elt_create(void *element);
/**
 * \brief Delete a list element structure
 * \param qe The structure to delete
 * \param element_free Pointer to a function to delete the element contained within the list element structure
 */
void list_elt_free(list_elt_t *qe, void (*element_free) (void *element));

/**
 * \brief Create a new list
 * \return The newly created list
 */
list_t *list_create(void);
/**
 * \brief Delete a list
 * \param list Pointer to the list to be deleted
 * \param element_free Pointer to a function to delete the elements contained within the list element structures that make up the list
 */
void list_free(list_t *list, void (*element_free)(void *element));
/**
 * \brief Push an element onto the list
 * \param list Pointer to the list to push an element on to
 * \param element Pointer to the element to add to the list
 */
void list_push_element(list_t *list, void *element);
/**
 * \brief Pop an element from the list
 * \param list Pointer to the list to pop an element from
 */
void *list_pop_element(list_t *list);

#endif
