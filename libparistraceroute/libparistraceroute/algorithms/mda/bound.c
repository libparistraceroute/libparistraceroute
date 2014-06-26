/**
 * bound.c
 * Author: Thomas Delacour
 * Description: Implementation of error bounding as described in 
 * the May 2007 Paris Traceroute workshop and April 2009 Infocom papers. 
 * Detailed explanation can be found at www.paris-traceroute.net/publications.
 */

#include <stdbool.h> // bool
#include <stdio.h>   // fprintf, sscanf
#include <stdlib.h>  // malloc, calloc, free
#include <string.h>  // memset
#include <math.h>    // log

#include "bound.h"

// We condiser a set of diagonal vectors (indexed by i) made of several cells (indexed by j)
#define PROBA_HOR(i, j)    ((long double)(j) / (i))              // Probability to follow a horizontal transition
#define PROBA_VER(i, j)    ((long double)((i) - (j) + 1) / (i)) // Probability to follow a vertical transition
#define NUM_PROBES(i, j)   ((i) + (j) - 1)                      // Translate position (i, j) into the corresponding number of probes
#define HSTART             2                                    // First two hypothesis (0 or 1 interfaces) are ignored

//--------------------------------------------------------------------------
// bound_state_t
//--------------------------------------------------------------------------

static bound_state_t * bound_state_create(size_t max_interfaces)
{
    bound_state_t * state;
    
    if (!(state = malloc(sizeof(bound_state_t)))) {
        goto ERR_STATE_MALLOC;
    }

    // Create parallel vectors contained in state
    if (!(state->first = malloc(max_interfaces * sizeof(probability_t)))) {
        goto ERR_FIRST_VECTOR;
    }

    if (!(state->second = malloc(max_interfaces * sizeof(probability_t)))) {
        goto ERR_SECOND_VECTOR;
    }

    return state;

    ERR_SECOND_VECTOR:
        free(state->first);
    ERR_FIRST_VECTOR:
        free(state);
    ERR_STATE_MALLOC:
        return NULL;
}

static void bound_state_free(bound_state_t * bound_state) 
{
    if (bound_state) {
        if (bound_state->first)  free(bound_state->first);
        if (bound_state->second) free(bound_state->second);

        free(bound_state);
    }
}

//--------------------------------------------------------------------------
// bound_t
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// private helper functions
//--------------------------------------------------------------------------

/**
 * \brief The inverse of the stopping condition, which is:
 * if at a state that is on the same vertical level as the stopping point to
 * be computed, AND the sum of the current state + the probabilities of
 * reaching a failing state beforehand is less than or equal to the given
 * confidence.
 */

inline static bool continue_condition(
    size_t        jstart, 
    size_t        hypothesis, 
    long double   cur_state, 
    bound_t     * bound
) {

    size_t i;
    long double pr_sum = 0.0;
    
    for (i = 0; i <= jstart + 1; ++i) {
        pr_sum += bound->pk_table[i];
    }
    return (jstart != hypothesis - 1 || 
            bound->confidence < pr_sum + cur_state);
}

/**
 * \brief Calculation of a single state. Calculation is the summation of two
 * parts: 1) the probability of reaching the state from a horizontal move
 *        2) the probability of reaching the state from a vertical move
 */

inline static probability_t calculate(bound_state_t * bound_state, size_t hypothesis, size_t j) {
    return bound_state->first[j]
         * PROBA_HOR(hypothesis, j)   //1
         + bound_state->second[j - 1]
         * PROBA_VER(hypothesis, j);  //2
}

inline static void swap(bound_state_t * bound_state) {
    probability_t * temp = bound_state->first;
    bound_state->first = bound_state->second;
    bound_state->second = temp;
}

/**
 * \brief For each new hypothesis' state space, initialize dummy "first" 
 * vector to all with probability_t 0.0 and initialize second vector
 * with probability 1.0 at first reachable  state - state(1,1)
 */

static probability_t init_state(bound_t * bound, bound_state_t * bound_state) {
    size_t j;

    for (j = 0; j < bound->max_n; ++j) {
        bound_state->first[j] = 0.0;
    }

    bound_state->second[0] = 0.0;
    bound_state->second[1] = 1.0;

    return 1.0;
}

bound_t * bound_create(double confidence, size_t max_interfaces)
{
    bound_t * bound;

    // Allocate and populate bound_t structure
    if (!(bound = malloc(sizeof(bound_t)))) goto ERR_BOUND_MALLOC;

    bound->confidence = confidence;
    bound->max_n = max_interfaces;

    // TODO comment about +1
    if (!(bound->nk_table = calloc(max_interfaces + 1, sizeof(size_t)))) {
        goto ERR_NK_TABLE_MALLOC;
    }

    if (!(bound->pk_table = calloc(max_interfaces + 1, sizeof(size_t)))) {
        goto ERR_PK_TABLE_MALLOC;
    }  

    if (!(bound->state = bound_state_create(max_interfaces))) {
        goto ERR_STATE;
    }

    return bound;

    ERR_STATE:
        free(bound->pk_table);
    ERR_PK_TABLE_MALLOC:
        free(bound->nk_table);
    ERR_NK_TABLE_MALLOC:
        free(bound);
    ERR_BOUND_MALLOC:
        return NULL;
}

void bound_build(bound_t * bound)
{
    size_t          hypothesis, i, j = 0, jstart; // Note: j does not require to be set to 0
    bound_state_t * state;
    probability_t   cur_state;

    // Handle potential nulls
    if (!bound || !(bound->nk_table) || !(bound->state)) {
        goto ERR_NULL_ARG;
    }
    state = bound->state;

    for (hypothesis = HSTART; hypothesis <= bound->max_n; ++hypothesis) {
        cur_state = init_state(bound, state);
        jstart = 2;

        // Walk horizontally accross state space
        for (i = 1; continue_condition(jstart, hypothesis, cur_state, bound); ++i) {
            jstart = (i == 2) ? 1 : jstart; // Awkward check required to get around state(1, 1) = 1.0 necessarily

            // Compute values and fill vector (vertically)
            for (j = jstart; j < hypothesis; ++j) {
                cur_state = calculate(state, hypothesis, j); 

                // If at a previously computed stopping point, enter
                // unreachable state (probability 0). Add probability of
                // failure at this level to pk_table
                if (NUM_PROBES(i, j) == (bound->nk_table)[j + 1]) {
                    jstart = j + 1;
                    state->second[j] = 0.0;
                    (bound->pk_table)[j + 1] = cur_state;
                } else {
                    state->second[j] = cur_state;
                }
            }
            swap(state);
        }
        bound->nk_table[hypothesis] = NUM_PROBES(i, j) - 2; // Sub 2 for increment of i and j after stopping condition
    }
    return;

    ERR_NULL_ARG:
        fprintf(stderr, "Provided bound struct contained null values or was itself null\n");
}

size_t bound_get_nk(bound_t * bound, size_t k)
{
    size_t ret = 0;

    if (bound) {
        if ((bound->nk_table) && (bound->max_n >= k)) {
            ret = (bound->nk_table)[k];
        }
    }
    return ret;
}

void bound_dump(bound_t * bound) 
{
    size_t i;

    for (i = 0; i <= bound->max_n; i++) {
        printf("%zu - %zu\n", i, bound->nk_table[i]);
    }
}

void bound_free(bound_t * bound)
{
    if (bound) {
        if (bound->pk_table) free(bound->pk_table);
        if (bound->nk_table) free(bound->nk_table);
        if (bound->state)    bound_state_free(bound->state);
        free(bound);
    }
}

int main(int argc, const char * argv[]) {
    long double confidence;
    size_t      interfaces;

//    sscanf(argv[1], "%Lf", &confidence);
//    sscanf(argv[2], "%d", &interfaces);
    confidence = 0.05;
    interfaces = 16;
    bound_t * bound = bound_create(confidence, interfaces);
    bound_build(bound);
    bound_dump(bound);
    bound_free(bound);
    return 0;
}

