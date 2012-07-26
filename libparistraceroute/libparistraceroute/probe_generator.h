/*
 * probe_generator.h
 *
 *  Created on: Jul 22, 2012
 *      Author: Julian Cromarty
 */

#ifndef PROBE_GENERATOR_H_
#define PROBE_GENERATOR_H_

#include <stdlib.h>

#include "probe.h"
#include "predicate.h"
#include "dynarray.h"



typedef enum predicate_bool_t {
	AND,
	OR,
	XOR,
	NOT
} predicate_bool_t;

typedef enum Node_type {
	NODE_OPERATOR,
	NODE_PREDICATE
} node_type_t;

typedef struct Generator_node {
	dynarray_t*				leaves;			//Array of leaves (generator_node_t*)
	int 					numLeaves;
	predicate_bool_t*		bool_operator;	//Only one or the other of these is used
	predicate_t*			predicate;		//The other will be null
} generator_node_t;

//Struct acting like a class for setting constraints
typedef struct Generator {
	dynarray_t*		 		predicate_roots; //dynamic array storing predicate tree roots (generator_node_t*)
	probe_t*				probe_skel;		//Probe skeleton that stores the current probe
} generator_t;

generator_node_t* generator_node_create();

//Add a predicate tree. Second arg is function pointer to predicate_and, predicate_or etc..
generator_t* add_predicate(generator_t*, void*, predicate_t*, ...);

//Assigns the first valid probe to the given probe_t pointer
//Return: True on success, false if no valid probes
//TODO: implement generator_get_first
bool generator_get_first(const generator_t*, probe_t*);

//Assigns the last valid probe to the given probe_t pointer
//Return: True on success, false if no valid probes
//TODO: implement generator_get_last
bool generator_get_last(const generator_t*, probe_t*);

//Updates the given probe_t* to point to the next valid probe
//Return: True on success, false if no more valid probes
//TODO: implement bool_get_next
bool generator_get_next(const generator_t*, probe_t*);

//Sets either a predicate or an operator as the content of a predicate tree node
//Nullifies both node pointers before setting new data to ensure only one or the other is ever set
void generator_node_set_content(node_type_t, void*);


#endif /* PROBE_GENERATOR_H_ */
