/*
 * predicate.h
 *
 *  Created on: Jul 24, 2012
 *      Author: Julian Cromarty
 */

#ifndef PREDICATE_H_
#define PREDICATE_H_

//Enum to hold the various types of predicate
typedef enum predicate_type_t {
	DEST_IP,
	SRC_IP,
	TTL_MIN,
	TTL_MAX
} predicate_type_t;

//Enum holding types of conditional operators
typedef enum operator_t {
	EQ,		//==
	LE,		//<=
	GE,		//>=
	LT,		//<
	GT,		//>
	NE		//!=
} operator_t;

//Structure of a predicate
typedef struct predicate_t {
	predicate_type_t condition;
	operator_t cond_operator;
	char* buffer;
} predicate_t;

predicate_t* predicate_create(void);

predicate_t* predicate_set_condition(predicate_t*, predicate_type_t);
predicate_t* predicate_set_operator(predicate_t*, operator_t);
predicate_t* predicate_set_data(predicate_t*, char*);

//Dumps predicate to stdout for debugging
void predicate_dump(predicate_t*);

#endif /* PREDICATE_H_ */
