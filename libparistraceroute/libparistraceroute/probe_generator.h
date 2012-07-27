/*
 * probe_generator.h
 *
 *  Created on: Jul 22, 2012
 *      Author: Julian Cromarty
 */

#ifndef PROBE_GENERATOR_H_
#define PROBE_GENERATOR_H_

#include <stdlib.h>
#include <stdbool.h>

#include "probe.h"
#include "predicate.h"
#include "dynarray.h"



typedef enum predicate_bool_t {
	AND,
	OR,
	XOR,
	NOT
} predicate_bool_t;

typedef enum node_type {
	NODE_BOOL,
	NODE_OPERATOR,
	NODE_CONSTANT,
	NODE_REFERENCE
} node_type_t;

typedef struct generator_node_t {
	void*			left;		//Pointer to left leaf
	void*			right;		//Pointer to right leaf
	node_type_t		type;		//Enum to say which of the 3 types of node is usea
	void*			content;	//Holds the node itself
} generator_node_t;

//Struct acting like a class for setting constraints
typedef struct generator_t {
	//dynarray_t*		 		predicate_roots; //dynamic array storing predicate tree roots (generator_node_t*)
	probe_t*				probe_skel;		//Probe skeleton that stores the current probe
	generator_node_t*		root_node;
} generator_t;

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

#endif /* PROBE_GENERATOR_H_ */
