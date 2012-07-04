#ifndef ALGORITHMS_MDA_H
#define ALGORITHMS_MDA_H

typedef enum {
    MDA_NEW_LINK
} mda_event_type_t;

typedef struct {
    mda_event_type_t type;
    void * data;
} mda_event_t;

#include "mda/data.h"
#include "mda/flow.h"
#include "mda/interface.h"

#endif
