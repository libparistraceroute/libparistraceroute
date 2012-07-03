#ifndef COMMON_H
#define COMMON_H

#define ELEMENT_FREE void (*)(void *)
#define ELEMENT_DUP void * (*)(void *)
#define ELEMENT_DUMP void (*)(void *)

#define MIN(x,y) (x<y)?x:y
#define MAX(x,y) (x>y)?x:y

double get_timestamp(void);
void print_indent(unsigned int indent);

#endif
