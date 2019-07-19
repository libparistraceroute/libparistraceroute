#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "common.h"

// Does the library do print results. Default is yes for compatibility purpose
bool _doPrint = true;

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

void setPrintMode(bool iDoPrint) {
		_doPrint = iDoPrint;
}

