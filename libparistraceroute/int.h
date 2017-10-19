#ifndef INT_H
#define INT_H

/**
 * Dup function for int pointer
 */
uint8_t *uint8_dup(uint8_t *i);


/**
 * Callback for int key for comparison function in the map.
 */
int uint8_compare(const uint8_t *element1, const uint8_t *element2);

#endif
