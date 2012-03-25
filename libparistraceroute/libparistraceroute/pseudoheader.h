#ifndef PSEUDOHEADER_H
#define PSEUDOHEADER_Hy

#define MAXBUF 10000

typedef struct {
    char data[MAXBUF];
    unsigned int size;
} pseudoheader_t;

#endif
