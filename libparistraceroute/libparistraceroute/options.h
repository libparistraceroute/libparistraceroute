#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdbool.h>   // bool
#include "optparse.h"  // optspec
#include "vector.h"    // vector

#define END_OPT_SPECS { .action = NULL}

typedef struct opt_spec opt_spec_t;

typedef struct {
    /**
     * A callback to handle collision between two options. If set to NULL (default value)
     * the options_t instance keep option1 and ignore option2.
     * \param option1 An opt_spec_t instance already stored in this options_t instance
     * \param option2 An opt_spec_t instance that we attempt to insert in this options_t instance
     * \return Must return true if the collision has been properly handled, false otherwise.
     */

    bool (* collision_callback)(
        opt_spec_t       * option1,
        const opt_spec_t * option2
    );

    vector_t * optspecs;  /**< A pointer to a vector structure containing optspecs instances */
} options_t;

/**
 * \brief Create the options structure containig the command line options
 * \param collision_callback A callback to handle collision between options
 * \return A pointer to options structure
 */

options_t * options_create(bool (* collision_callback)(opt_spec_t * option1, const opt_spec_t * option2));

/**
 * \brief Add options to an array of options
 * \param options A pointer to an options_t structure containing the array of options to fill
 * \param option A pointer to an array of options to add
 * \return true iif successful
 */

bool options_add_optspecs(options_t * options, opt_spec_t * option);

/**
 * \brief Add one option to an array of options
 * \param options A pointer to an options_t structure containing the array of options to fill
 * \param option A pointer to the  options to add
 * \return true iif successful
 */

bool options_add_optspec(options_t * options, opt_spec_t * option);

/**
 * \brief Add the common options (help and version) to an array of options
 * \param A pointer to an options_t structure containing the array of options to fill
 * \param A string containing the data to put in the version message
 * \return true iif successful
 */

bool options_add_common(options_t * options, const char * version_data);

/**
 * \brief Get the number of otpons contained in an options_t instance
 * \param A pointer to an options_t structure
 * \return the number of options
 */

size_t options_get_num_options(const options_t * options);

/**
 * \brief Rename the short form of an opt_spec_t instance
 * \param A pointer to an opt_spec_t instance
 * \param String containing the new short form
 * \return true iif successful
 */

bool option_rename_sf(opt_spec_t * option, char * sf);

/**
 * \brief Rename the long form of an opt_spec_t instance
 * \param A pointer to an opt_spec_t instance
 * \param String containing the new long form
 * \return true iif successful
 */

bool option_rename_lf(opt_spec_t * option, char * lf);

/**
 * \brief Dump an options_t instance
 * \param options Pointer to
 */
void options_dump(const options_t * options);

/**
 * \brief Parse command line options forcing the options before the argument
 * \param options Pointer to options used
 * \param usage a string containig the message to put when no argument passed
 * \param args vector containing the arguments passed
 */
int options_parse(options_t * options, const char * usage, char ** args);

#endif
