#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "common.h"

// Note: for measuring intervals !
//#include <time.h>
//struct timespec ts;
//clock_gettime(CLOCK_MONOTONIC, &ts);

double get_timestamp(void)
{
    struct timeval tim;
    double t;
    
    gettimeofday(&tim, NULL);
    t = tim.tv_sec + (tim.tv_usec / 1000000.0);

    return t;
}

void print_indent(unsigned int indent)
{
    int i;
    for(i = 0; i < indent; i++)
        printf("    ");
}

