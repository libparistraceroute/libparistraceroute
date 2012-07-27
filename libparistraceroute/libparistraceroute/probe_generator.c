/*
 * probe_generator.c
 *
 *  Created on: Jul 22, 2012
 *      Author: Julian Cromarty
 */

#include "probe_generator.h"

generator_t* generator_create() {
	generator_t* g = malloc(sizeof(generator_t));
	if (!g)
		goto ERR_GENERATOR;

	/*g->predicate_roots = dynarray_create();
	if (!g->predicate_roots)
		goto ERR_ROOT;*/

	g->probe_skel = probe_create();
	if (!g->probe_skel)
		goto ERR_PROBE;

	return g;		//Success!

	ERR_PROBE: //dynarray_free(g->predicate_roots, NULL );
	ERR_ROOT: free(g);
	ERR_GENERATOR: return NULL ;
}

int generator_set_next_node(generator_node_t* current_node, generator_node_t* new_node)	{
	if (current_node->type == NODE_BOOL)	{
		if (!current_node->left)	{
			current_node->left = new_node;
			return 0;
		}
		else if (!current_node->right)	{
			current_node->right = new_node;
			return 0;
		}
	}
	if (current_node->left && generator_set_next_node(current_node->left, new_node) == 0)	{
		return 0;
	}
	else if (current_node->right && generator_set_next_node(current_node->right, new_node) == 0)	{
		return 0;
	}
	return -1;
}

generator_t* add_predicate(generator_t* gen, predicate_bool_t* bool_operator, void* left, node_type_t left_type, operator_t* operator, void* right, node_type_t right_type)	{
	generator_node_t* bool_node, left_node, operator_node, right_node;
	bool_node = malloc(sizeof(generator_node_t));
	if (!bool_node) goto ERR_BOOL;
	left_node = malloc(sizeof(generator_node_t));
	if (!left_node) goto ERR_LEFT;
	operator_node = malloc(sizeof(generator_node_t));
	if (!operator_node) goto ERR_OPERATOR;
	right_node = malloc(sizeof(generator_node_t));
	if (!right_node) goto ERR_RIGHT;

	bool_node->content = bool_operator;
	bool_node->left = operator_node;
	bool_node->type = NODE_BOOL;
	operator_node->content = operator;
	operator_node->left = left_node;
	operator_node->right = right_node;
	operator_node->type = NODE_OPERATOR;
	left_node->content = left;
	left_node->type = left_type;
	right_node->content = right;
	right_node->type = right_type;

	generator_set_next_node(gen, bool_node);	//TODO: check for success

	return gen;
	ERR_RIGHT:
	free(operator_node);
	ERR_OPERATOR:
	free(left_node);
	ERR_LEFT:
	free(bool_node);
	ERR_BOOL:
	return NULL;
}

bool generator_get_first(const generator_t* g, probe_t* p) {
	//Make probe
	probe_t* probe_skel = probe_create();

	generator_node_t* root_node;



}

