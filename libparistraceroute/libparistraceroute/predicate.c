/*
 * predicate.c
 *
 *  Created on: Jul 24, 2012
 *      Author: Julian Cromarty
 */

#include <stdlib.h>

#include "predicate.h"

predicate_t* predicate_create(void)	{
	predicate_t *predicate = malloc(sizeof(predicate_t));
	if (!predicate) return NULL;

	/*predicate->buffer = buffer_create();
	if (!predicate->buffer)	{
		free(predicate);
		return NULL;
	}*/

	return predicate;
}

predicate_t* predicate_set_condition(predicate_t* p, predicate_type_t type) {
	p->condition = type;
	return p;
}

predicate_t* predicate_set_operator(predicate_t* p, operator_t o)	{
	p->cond_operator = o;
	return p;
}

predicate_t* predicate_set_data(predicate_t* p, char* data) {
	p->buffer = data;
	return p;
}

//Dumps a predicate to stdout
void predicate_dump(predicate_t* p)	{
	char* c;
	char* o;
	switch (p->condition)	{
	case DEST_IP:	c = "Dest IP";
					break;
	case SRC_IP:	c = "Src IP";
					break;
	case TTL_MIN:	c = "TTL Min";
					break;
	case TTL_MAX:	c = "TTL Max";
					break;
	default:		c = "Unknown";
					break;
	}

	switch (p->cond_operator)	{
	case EQ:
		o = "==";
		break;
	case LE:
		o = "<=";
		break;
	case GE:
		o = ">=";
		break;
	case LT:
		o = "<";
		break;
	case GT:
		o = ">";
		break;
	case NE:
		o = "!=";
		break;
	default:
		o = "";
		break;
	}

	printf("Condition:%s\n",c);
	printf("Operator:%s\n",o);
	printf("Value:%s\n",p->buffer);
}
