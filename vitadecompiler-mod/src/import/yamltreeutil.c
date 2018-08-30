/*

Copyright (C) 2016, David "Davee" Morgan

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include "yamltreeutil.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

int yaml_iterate_mapping(yaml_node *node, mapping_functor functor, void *userdata)
{
	// check we have a scalar
	if (node->type != NODE_MAPPING)
		return -1;
	
	yaml_mapping *module = &node->data.mapping;
	
	for (int i = 0; i < module->count; ++i)
	{
		if (functor(module->pairs[i]->lhs, module->pairs[i]->rhs, userdata) < 0)
			return -2;
	}
	
	return 0;
}

int yaml_iterate_sequence(yaml_node *node, sequence_functor functor, void *userdata)
{
	// check we have a scalar
	if (node->type != NODE_SEQUENCE)
		return -1;
	
	yaml_sequence *module = &node->data.sequence;
	
	for (int i = 0; i < module->count; ++i)
	{
		if (functor(module->nodes[i], userdata) < 0)
			return -2;
	}
	
	return 0;
}

int process_32bit_integer(yaml_node *node, uint32_t *nid) 
{	
	// check we have a scalar
	if (node->type != NODE_SCALAR)
		return -1;
	
	yaml_scalar *scalar = &node->data.scalar;
	
	char *endptr = NULL;
	uint32_t res = strtoul(scalar->value, &endptr, 0);
	
	if (*endptr) 
	{
		return (res == ULONG_MAX) ? (-2) : (-3);
	}
	
	*nid = res;
	return 0;
}

int process_boolean(yaml_node *node, uint32_t *boolean) 
{	
	// check we have a scalar
	if (node->type != NODE_SCALAR)
		return -1;
	
	yaml_scalar *scalar = &node->data.scalar;
	
	if (strcmp(scalar->value, "true") == 0)
	{
		*boolean = 1;
	}
	else if (strcmp(scalar->value, "false") == 0)
	{
		*boolean = 0;
	}
	else
	{
		// invalid input
		return -2;
	}
	
	return 0;
}

int process_bool(yaml_node *node, bool *boolean) 
{	
	uint32_t cast = 0;
	int res = process_boolean(node,&cast);
	
	if(!res){
		*boolean = cast == 1;
	}
	
	return res;
}

int process_string(yaml_node *node, const char **str) 
{
	// check we have a scalar
	if (node->type != NODE_SCALAR)
		return -1;
	
	yaml_scalar *scalar = &node->data.scalar;
	*str = scalar->value;
	return 0;
}

int is_scalar(yaml_node *node)
{
	return (node->type == NODE_SCALAR);
}

int is_mapping(yaml_node *node)
{
	return (node->type == NODE_MAPPING);
}

int is_sequence(yaml_node *node)
{
	return (node->type == NODE_SEQUENCE);
}
