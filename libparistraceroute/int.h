#ifndef INT_H
#define INT_H

/**
 * @brief Dup function for int pointer.
 * @param i Pointer that will be duplicated.
 * @return The new allocated pointer.
 */

uint8_t * uint8_dup(uint8_t * i);

/**
 * @brief Callback for int key for comparison function in the map.
 * @param element1 First element to be compared.
 * @param element2 Second element to be compared.
 * @return int
 */

int uint8_compare(const uint8_t * element1, const uint8_t * element2);

#endif
