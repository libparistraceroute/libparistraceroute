#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "common.h"

// Note: for measuring intervals !
//#include <time.h>
//struct timespec ts;
//clock_gettime(CLOCK_MONOTONIC, &ts);

double get_timestamp()
{
    struct timeval tim;
    
    gettimeofday(&tim, NULL);
    return tim.tv_sec + (tim.tv_usec / 1000000.0);
}

void print_indent(unsigned int indent)
{
    unsigned int i;
    for(i = 0; i < indent; i++) {
        printf("    ");
    }
}

