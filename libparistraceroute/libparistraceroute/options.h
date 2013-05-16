#ifndef OPTIONS_H
#define OPTIONS_H


#include <stdbool.h> 

#include "optparse.h"
#include "vector.h"

typedef struct {
    void    (* options_collision_callback)(struct opt_spec * option1, struct opt_spec * option2);
    vector_t * vector;
} options_t;


options_t * options_create(void (* callback)(struct opt_spec * option1, struct opt_spec * option2));



void options_add_opt_specs(options_t * options_to_fill, struct opt_spec * option_to_add, size_t num_options_to_add);

void options_add_opt_spec(options_t * options_to_fill, struct opt_spec * option_to_add);

//void option_replace(struct opt_spec * option1, struct opt_spec * option2);

void option_rename(struct opt_spec * option, char sf[], char lf[]);

//void option_rename_2(struct opt_spec * option1, struct opt_spec * option2); 

#endif
