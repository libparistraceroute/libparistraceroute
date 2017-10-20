#include <stdlib.h> // malloc
#include <stdint.h> // uint8_t

#include "int.h"



uint8_t * uint8_dup(uint8_t * i) {
    uint8_t * j = malloc(sizeof(uint8_t));
    *j = *i;
    return j;
}


int uint8_compare(const uint8_t * element1, const uint8_t * element2) {
    if (*element1 < *element2) {
        return -1;
    } else if (*element1 > *element2) {
        return 1;
    }
    return 0;
}
