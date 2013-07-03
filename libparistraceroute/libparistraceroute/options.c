#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "options.h"

/**
 * \brief Compare two opt_spect_t instances.
 * \param option1 The first opt_spect_t instance.
 * \param option2 The second opt_spect_t instance.
 * \return true iif the both options are equal.
 */

static bool optspec_is_same(const opt_spec_t * option1, const opt_spec_t * option2) {
    return (option1 && option2
        && *(option1->action) == *(option2->action)
        && option1->sf
        && option2->sf
        && strcmp(option1->sf, option2->sf) == 0
        && option1->lf
        && option2->lf
        && strcmp(option1->lf, option2->lf) == 0);
}

/**
 * \brief Print an opt_spect_t instance.
 * \param option The opt_spect_t instance we want to print.
 */

static void optspec_dump(const opt_spec_t * option) {
    printf("option : %s\t%s\n", option->sf, option->lf);
}

/**
 * \brief Check whether an optspec_t instance collides with one containe in an
 *    options_t instance.
 * \param options An options_t instance carrying a set of options.
 * \param option An opt_spect_t instance we compare with the ones stored in options.
 * \return A pointer to a colliding options stored in vector, NULL if not found.
 */

static opt_spec_t * options_search_colliding_option(options_t * options, opt_spec_t * option) {
    size_t       i;
    opt_spec_t * options_i;

    for (i = 0; i < vector_get_num_cells(options->optspecs); i++) {
        options_i =  vector_get_ith_element(options->optspecs, i);
        if (optspec_is_same(options_i, option)) {
            return options_i;
        }
    }
    return NULL;
}

options_t * options_create(bool (* collision_callback)(opt_spec_t * option1, const opt_spec_t * option2))
{
    options_t * options;

    if (!(options = malloc(sizeof(options_t)))) {
        goto ERR_MALLOC;
    }

    if (!(options->optspecs = vector_create(
        sizeof(opt_spec_t),
        NULL,
        (void *)(const opt_spec_t *) optspec_dump
    ))) {
        goto ERR_VECTOR_CREATE;
    }

    options->collision_callback = collision_callback;
    options->optspecs->cells = (opt_spec_t *) options->optspecs->cells;
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

bool options_add_optspecs(options_t * options, opt_spec_t * optspecs)
{
    opt_spec_t * optspec;
    bool   ret = true;

    for (optspec = optspecs; optspec->action && ret; optspec++) {
        ret &= options_add_optspec(options, optspec);
    }
    return ret;
}

bool options_add_optspec(options_t * options, opt_spec_t * option)
{
    bool         ret;
    opt_spec_t * colliding_option = options_search_colliding_option(options, option);

    if (!colliding_option) {
        // No collision, add this option
        ret = vector_push_element(options->optspecs, option);
    } else if (options->collision_callback) {
        // Collision detected, call collision_callback
        ret = options->collision_callback(colliding_option, option);

        // Unhandled collision, print an error message
        if (!ret) {
            fprintf(
                stderr,
                "W: option collision detected (%s, %s)\n",
                option->sf,
                option->lf
            );
        }
    }

    // We simply ignore option, which is not added to options
    return true;
}

bool options_add_common(options_t * options, const char * version_data)
{
    bool ret = false;

    if (options && version_data) {
        opt_spec_t common_options[] = {
            {opt_help,       "h", "--help"   , OPT_NO_METAVAR, OPT_NO_HELP, OPT_NO_DATA},
            {opt_version,    "V", "--version", OPT_NO_METAVAR, OPT_NO_HELP, version_data},
            {OPT_NO_ACTION},
            END_OPT_SPECS
        };
        options_add_optspecs(options, common_options);
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

int options_parse(options_t * options, const char * usage, char ** args)
{
    opt_options1st();
    return opt_parse(usage, (struct opt_spec *)(options->optspecs->cells), args);
}
