#include <stdlib.h> // malloc
#include <stdint.h> // uint8_t

#include "int.h"


/**
 * Dup function for int pointer
 */
int *int_dup(int *i) {
    int *j = malloc(sizeof(int *));
    *j = *i;
    return j;
}


/**
 * Callback for int key for comparison function in the map.
 */
int compare(const uint8_t *element1, const uint8_t *element2) {
    if (*element1 < *element2) {
            return -1;
        }
    else if (*element1 > *element2) {
            return 1;
        }
    return 0;
}
