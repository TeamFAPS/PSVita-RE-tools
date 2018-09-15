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

#ifndef YAMLTREE_H
#define YAMLTREE_H

#include  <stdio.h>

typedef enum 
{
	NODE_SCALAR,
	NODE_MAPPING,
	NODE_SEQUENCE
} yaml_node_type;

typedef struct
{
	size_t line;
	size_t column;
} yaml_position;

typedef struct 
{
	const char *value;
	size_t len;
} yaml_scalar;

typedef struct 
{
	size_t count;
	struct yaml_node **nodes;
} yaml_sequence;

typedef struct 
{
	struct yaml_node *lhs;
	struct yaml_node *rhs;
} yaml_node_pair;

typedef struct 
{
	size_t count;
	yaml_node_pair **pairs;
} yaml_mapping;

typedef struct yaml_node 
{
	yaml_position position;
	yaml_node_type type;
	union 
	{
		yaml_scalar scalar;
		yaml_mapping mapping;
		yaml_sequence sequence;
	} data;
} yaml_node;

typedef yaml_node yaml_document;

typedef struct 
{
	size_t count;
	yaml_document **docs;
} yaml_tree;

typedef struct
{
	char *problem;
} yaml_error;

yaml_tree *parse_yaml_stream(FILE *input, yaml_error *error);
void free_yaml_tree(yaml_tree *tree);
const char *node_type_str(yaml_node *node);

#endif // YAMLTREE_H
