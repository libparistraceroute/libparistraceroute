#ifndef WORKSHOP_BOUND_H
#define WORKSHOP_BOUND_H

#include <stddef.h>
/**
 * bound.h
 * Author: Thomas Delacour
 * Description: header file to support various workshopBound versions
 */

typedef long double probability_t;

/**
 * \struct bound_state_t
 * \brief Structure containing references to dynamic vectors
 *      used to build state space. Done by alternating first
 *      and second, each of which represent a diagonal section
 *      of the state space.
 */

typedef struct {
    probability_t * first;
    probability_t * second;
} bound_state_t;

/** 
 * \struct bound_t
 * \brief Structure used to store data relevant to bound
 */

typedef struct {
    double          confidence; /**< Desired failure confidence */
    size_t          max_n;      /**< Max assumed branching at an interface */
    size_t        * nk_table;   /**< Stored stopping points */
    probability_t * pk_table;   /**< Temp stored probabilities at stopping
                                     point states for each hypothesis */
    bound_state_t * state;      /**< Reference to memory (vector_ts) used to 
                                     build state */
} bound_t;


/**
 * \brief Create a bound_t structure
 * \param confidence User-specified failure confidence
 * \param max_interfaces User-specified max branching at an interface
 * \return Reference to bound_t structure
 */

bound_t * bound_create(double confidence, size_t max_interfaces);

/**
 * \brief Compute stopping points
 * \param bound Reference to bound_t structure
 */

void bound_build(bound_t * bound, size_t end);

/**
 * \brief Get stopping point for a given hypothesis
 * \param bound Reference to bound_t structure
 * \param k Given hypothesis (number of presumed children)
 * \return Associated stopping point (probes to send)
 */

size_t bound_get_nk(bound_t * bound, size_t k);

/**
 * \brief Print all stopping points
 * \param bound Reference to bound_t structure
 */

void bound_dump(bound_t * bound);

/**
 * \brief Free bound_t structure
 * \param bound Reference to bound_t structure
 */

void bound_free(bound_t * bound);

#endif
