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

#ifndef YAMLTREEUTIL_H
#define YAMLTREEUTIL_H

#include "yamltree.h"
#include <stdint.h>
#include <stdbool.h>

typedef int (* mapping_functor)(yaml_node *parent, yaml_node *child, void *userdata);
typedef int (* sequence_functor)(yaml_node *entry, void *userdata);

int yaml_iterate_mapping(yaml_node *node, mapping_functor functor, void *userdata);
int yaml_iterate_sequence(yaml_node *node, sequence_functor functor, void *userdata);

int process_32bit_integer(yaml_node *node, uint32_t *nid);
int process_boolean(yaml_node *node, uint32_t *boolean);
int process_bool(yaml_node *node, bool *boolean);
int process_string(yaml_node *node, const char **str);

int is_scalar(yaml_node *node);
int is_mapping(yaml_node *node);
int is_sequence(yaml_node *node);

#endif // YAMLTREEUTIL_H
