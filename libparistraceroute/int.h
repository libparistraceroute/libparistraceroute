#ifndef INT_H
#define INT_H

/**
 * Dup function for int pointer
 */
int *int_dup(int *i);


/**
 * Callback for int key for comparison function in the map.
 */
int compare(const uint8_t *element1, const uint8_t *element2);

#endif
