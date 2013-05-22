#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "options.h"

/**
 *
 */

static bool option_is_same(const opt_spec_t * option1, const opt_spec_t * option2) {
    return (option1 && option2
        && *(option1->action) == *(option2->action)
        && strcmp(option1->sf, option2->sf) == 0
        && strcmp(option1->lf, option2->lf) == 0);
}


void option_dump(const opt_spec_t * option) {
    printf("option : sf ->%s , lf->%s \n", option->sf, option->lf);
}

/**
 * \return A pointer to a colliding options stored in vector, NULL if not found
 */

static opt_spec_t * options_search_colliding_option(options_t * options, opt_spec_t * option) {
    size_t       i;
    opt_spec_t * options_i = NULL;

    for (i = 0; i < vector_get_num_cells(options->optspecs); i++) {
        options_i =  vector_get_ith_element(options->optspecs, i);
        if (option_is_same(options_i, option)) {
            fprintf(stderr, "W: option collision detected\n");
            printf("----option 1----\n");
            option_dump(options_i);
             printf("----option 2----\n");
            option_dump(option);
            break;
        } else { 
            options_i = NULL;
        }
    }
    return options_i;
}

options_t * options_create(void (* callback)(opt_spec_t * option1, opt_spec_t * option2))
{
    options_t * options;

    if (!(options = malloc(sizeof(options_t)))) {
        goto ERR_MALLOC;
    }

    if (!(options->optspecs = vector_create(sizeof(opt_spec_t), NULL, option_dump))) {
        goto ERR_VECTOR_CREATE;
    }

    options->collision_callback = callback;
    options->optspecs->cells = (opt_spec_t *)(options->optspecs->cells);
    return options;

ERR_VECTOR_CREATE:
    free(options);
ERR_MALLOC:
    return NULL;
}

void options_dump(const options_t * options) {
    printf("---options---\n");
    vector_dump(options->optspecs);
}


bool options_add_options(options_t * options, opt_spec_t * optspecs, size_t num_options)
{
    size_t i;

    for (i = 0; i < num_options; i++) {
        options_add_option(options, optspecs + i);
        
    }
    return true; 
}

bool options_add_option(options_t * options, opt_spec_t * option)
{
    opt_spec_t * colliding_option = options_search_colliding_option(options, option);

    if (!colliding_option) {
        // No collision, add this option
        return vector_push_element(options->optspecs, option);
    } else if (options->collision_callback) {
        // Collision detected, call collision_callback
        options->collision_callback(colliding_option, option);
        return true;
    }
    return false;
}

bool options_add_common(options_t * options, const char * version_data)
{   
    bool ret = false;

    if(options && version_data) {
        opt_spec_t common_options[] = {
            {opt_help,       "h", "--help"   , OPT_NO_METAVAR, OPT_NO_HELP, OPT_NO_DATA},
            {opt_version,    "V", "--version", OPT_NO_METAVAR, OPT_NO_HELP, version_data},
            {OPT_NO_ACTION}
        };
        options_add_options(options, common_options, 3);
        ret = true;
    }
    return ret;
}

bool option_rename_sf(opt_spec_t * option, char * sf) {
    return (option->sf = strdup(sf)) != NULL;
}

bool option_rename_lf(opt_spec_t * option, char * lf) {
    return (option->lf = strdup(lf)) != NULL;
}

/*
void option_replace(opt_spec_t * option1, opt_spec_t * option2) {
    option_rename_1(option1,option2);
     option2 = option2 + 1; 
    printf("first option replaced\n");
}

void option_rename_2(opt_spec_t * * option1, opt_spec_t * * option2) {
    option_rename_1(option2,option1);
}
*/
