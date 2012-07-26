/*
 * probe_generator.c
 *
 *  Created on: Jul 22, 2012
 *      Author: Julian Cromarty
 */

#include "probe_generator.h"

generator_t* generator_create()	{
	generator_t* g = malloc(sizeof(generator_t));
	if (!g) 					goto ERR_PROBE;

	g->predicate_roots = dynarray_create();
	if (!g->predicate_roots)	goto ERR_ROOT;

	g->probe_skel = probe_create();
	if (!g->probe_skel)			goto ERR_PROBE;

	return g;		//Success!

ERR_PROBE:
	dynarray_free(g->predicate_roots,NULL);
ERR_ROOT:
	free(g);
ERR_GENERATOR:
	return NULL;
}

generator_node_t* generator_node_create()	{
	generator_node_t* g = malloc(sizef(generator_node_t));
	if (!g)	goto ERR_NODE;

	g->leaves = dynarray_create();
	if (!g->leaves) goto ERR_LEAVES;

	return g;

ERR_LEAVES:
	free(g);
ERR_NODE:
	return NULL;
}

void generator_node_set_content(generator_node_t* node, node_type_t type, void* content) {
	//wipe existing content if it exists so that node only ever has one type of content
	if(node->bool_operator) free(node->bool_operator);
	if(node->predicate)	free(node->predicate);

	if (type == NODE_OPERATOR)	{
		node->bool_operator = (predicate_bool_t*) content;
	}
	else if (type == NODE_PREDICATE)	{
		node->predicate = (predicate_t*) content;
	}
}

bool generator_get_first(const generator_t* g, probe_t* p)	{
	//Make probe
	probe_t* probe_skel = probe_create();

	generator_node_t* root_node;

	//Marc-Olivier/jordan/anyone else. I'm still working on this bit. Probably going to do it
	//with a depth first search style approach in a recursive function to allow for an arbitrary
	//depth of predicates
	for (int i = 0; i < dynarray_get_size(g->predicate_roots); i++)	{
		root_node = dynarray_get_ith_element(g->predicate_roots, i);	//Get next predicate
		if (root_node->)
	}



}
