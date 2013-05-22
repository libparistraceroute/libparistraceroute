#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdbool.h>   // bool
#include "optparse.h"  // optspec
#include "vector.h"    // vector

typedef struct opt_spec opt_spec_t;

typedef struct {
    void (* collision_callback)(
        opt_spec_t * option1,
        opt_spec_t * option2
    ); /**< A callback to handle collision between two options */

    vector_t * optspecs;  /**< A pointer to a vector structure containing optspecs instances */
} options_t;

/**
 * \brief Create the options structure containig the command line options
 * \param collision_callback A callback to handle collision between options
 * \return A pointer to options structure
 */

options_t * options_create(void (* collision_callback)(opt_spec_t * option1, opt_spec_t * option2));

/**
 * \brief Add options to an array of options
 * \param options A pointer to an options_t structure containing the array of options to fill
 * \param option A pointer to an array of options to add
 * \param num_options The number of options to add
 */

// TODO return bool
bool options_add_options(options_t * options, opt_spec_t * option, size_t num_options);

/**
 * \brief Add one option to an array of options
 * \param options A pointer to an options_t structure containing the array of options to fill
 * \param option A pointer to the  options to add
 */

// TODO return bool
bool options_add_option(options_t * options, opt_spec_t * option);

/**
 * \brief Add the common options (help and version) to an array of options
 * \param A pointer to an options_t structure containing the array of options to fill
 * \param A string containing the data to put in the version message
 */

// TODO return bool
bool options_add_common(options_t * options, const char * version_data);

size_t options_get_num_options(const options_t * options);

//void option_replace(opt_spec_t * option1, opt_spec_t * option2);

void option_rename(opt_spec_t * option, char sf[], char lf[]);

//void option_rename_2(opt_spec_t * option1, opt_spec_t * option2); 

/**
 * \brief Dump an options_t instance
 * \param options Pointer to 
 */
void options_dump(const options_t * options);


#endif
