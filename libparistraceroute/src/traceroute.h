#ifndef H_PATR_TRACEROUTE
#define H_PATR_TRACEROUTE

#include "proto.h"
#include "algo.h"
#include "output.h"
/* macros ============================================================== */
/* constants =========================================================== */
/* types =============================================================== */
/* structures ========================================================== */
/* internal public functions =========================================== */
/* entry points ======================================================== */
/* public variables ==================================================== */
/* inline public functions  ============================================ */

/**
 * creates an instance of traceroute with the specified caracteristics
 * @param proto : the protocol used in the probes
 * @param algo : the algorithm used 
 * @param output the kind of output decided
 * @param fields the options as specified by the instance (she must perform transformations to get this kind)
 * @return an instance of traceroute ?
 */
int create_traceroute(char* proto, algo_s algo, output_s output, field_s fields);

//fonction pour traiter les options ( clif ou getopt? et transfo en la struct field), sauf si se fait au niv instance)
#endif
