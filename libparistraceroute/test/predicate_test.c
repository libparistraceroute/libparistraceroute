/*
 * predicate_test.c
 *
 *  Created on: Jul 24, 2012
 *      Author: jooles
 */

#include <stdlib.h>
#include "predicate.h"

int main (int argc, char** argv)	{
	predicate_t* p = predicate_create();
	predicate_set_condition(p, DEST_IP);
	predicate_set_operator(p, EQ);
	predicate_set_data(p, "1.2.3.4");

	predicate_dump(p);
	return 0;
}
