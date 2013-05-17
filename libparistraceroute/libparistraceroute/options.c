#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "options.h"



options_t * options_create(void (* callback)(struct opt_spec * option1, struct opt_spec * option2)) {
    
    options_t * options = malloc(sizeof(options_t));
    if(options){
            options->options_collision_callback = callback;
            options->vector = vector_create(sizeof(struct opt_spec));
    }
    return options;
}


static bool option_is_same(const struct opt_spec * option1, const struct opt_spec * option2) {
    if(option1 && option2){
        if(*(option1->action) == *(option2->action)) {
            if(!strcmp(option1->sf, option2->sf)) {
                if(!strcmp(option1->lf, option2->lf)) {
                    return true;
                } else { 
                    return false;
                } } else {
                    return false;
                } } else {
                    return false;
                } } else {
                    printf("fail to compare the 2 opt_spec structures\n");
                    return false;
                }

}

static struct opt_spec *  option_can_be_added(vector_t * vector, struct opt_spec * option) {
    unsigned int i;
    struct opt_spec * collision = NULL;
    for(i = 0; i < vector->num_cells; i++) {
        if(option_is_same((vector->cells + i), option)) {
                collision = (vector->cells + i);
                printf("W:option collision detected : first option< sf:%s, lf:%s> ; second option< sf:%s, lf:%s> \n", 
                        collision->sf,
                        collision->lf,
                        option->sf,
                        option->lf);
                break;
                }
                }
        return collision;
}

void options_add_opt_specs(options_t * options_to_fill, struct opt_spec * option_to_add, size_t num_options_to_add) {
    unsigned int i = 0;
    struct opt_spec * collisional_option = NULL;
   while(i < num_options_to_add) {
         collisional_option = option_can_be_added(options_to_fill->vector, (option_to_add + i));
             if(!collisional_option) {
            vector_push_element(options_to_fill->vector, option_to_add + i);
            i++; 
         } else {
             if (options_to_fill->options_collision_callback) {
                  options_to_fill->options_collision_callback(collisional_option, (option_to_add + i));
             } else {
                 i++; 
             }
         }
   }
}

void options_add_opt_spec(options_t * options_to_fill, struct opt_spec * option_to_add) {
    
    options_add_opt_specs(options_to_fill, option_to_add, 1);
}

void options_add_common(options_t *options_to_fill, char * version_data) {

    struct opt_spec common_options[] = {
        {opt_help,       "h", "--help"   , OPT_NO_METAVAR, OPT_NO_HELP, OPT_NO_DATA},
        {opt_version,    "V", "--version", OPT_NO_METAVAR, OPT_NO_HELP, version_data},
        OPT_NO_ACTION
    };

    options_add_opt_specs(options_to_fill, common_options, 3);
}

//TOFIX option rename make segfault 
void option_rename(struct opt_spec * option, char sf[], char lf[]) {
    
    if(option && sf && lf) {
    option->sf = strcpy(option->sf, sf);
    option->lf = strcpy(option->lf, lf);
    } else {
        printf("fail to rename option\n");
    }
}
/*
void option_replace(struct opt_spec * option1, struct opt_spec * option2) {
    option_rename_1(option1,option2);
     option2 = option2 + 1; 
    printf("first option replaced\n");
}

void option_rename_2(struct opt_spec * option1, struct opt_spec * option2) {
    option_rename_1(option2,option1);
}
*/
