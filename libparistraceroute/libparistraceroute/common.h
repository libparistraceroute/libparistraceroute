#ifndef COMMON_H
#define COMMON_H

/**
 * \brief Type related to a *_free() function
 */

#define ELEMENT_FREE void   (*)(void *)

/**
 * \brief Type related to a *_dup() function
 */

#define ELEMENT_DUP  void * (*)(void *)

/**
 * \brief Type related to a *_dump() function
 */

#define ELEMENT_DUMP void   (*)(void *)

/**
 * \brief Type related to a *_compare() function
 */

#define ELEMENT_COMPARE int (*)(const void *, const void *)

/**
 * \brief Macro returning the minimal value of two elements
 * \param x The left operand
 * \param x The right operand
 * \return The min of x and y
 */

#define MIN(x, y) (x < y) ? x : y

/**
 * \brief Macro returning the maximal value of two elements
 * \param x The left operand
 * \param x The right operand
 * \return The max of x and y
 */

#define MAX(x, y) (x > y) ? x : y

/**
 * \return The current timestamp (in seconds)
 */

double get_timestamp(void);

/**
 * \bruef Print some space characters
 * \param indent The number of space characters to print
 *   in the standard output 
 */

void print_indent(unsigned int indent);

#endif
