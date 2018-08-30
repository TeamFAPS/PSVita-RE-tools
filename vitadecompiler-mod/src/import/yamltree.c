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

#include "yamltree.h"
#include <yaml.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef struct 
{
	yaml_parser_t parser;
	yaml_event_t event;
	yaml_event_t next_event;
	yaml_error *error;
} parser_context;

static yaml_node *process_node(parser_context *ctx);

char *format_error_string(parser_context *ctx)
{
	assert(ctx->parser.error != YAML_NO_ERROR);
	char *ptr;
	
	switch (ctx->parser.error)
	{
		case YAML_MEMORY_ERROR:
			asprintf(&ptr, "libyaml: failed to allocate or reallocate a block of memory.");
			break;
			
		case YAML_READER_ERROR:
			if (ctx->parser.problem_value != -1)
				asprintf(&ptr, "libyaml: reader error: '%s:#%X' at line %d, column %d.", ctx->parser.problem, ctx->parser.problem_value, ctx->parser.problem_mark.line, ctx->parser.problem_mark.column);
			else
				asprintf(&ptr, "libyaml: reader error: '%s' at line %d, column %d.", ctx->parser.problem, ctx->parser.problem_mark.line, ctx->parser.problem_mark.column);
			break;
			
		case YAML_SCANNER_ERROR:
			asprintf(&ptr, "libyaml: scanner error: '%s' at line %d, column %d.", ctx->parser.problem, ctx->parser.problem_mark.line, ctx->parser.problem_mark.column);
			break;
			
		case YAML_PARSER_ERROR:
			asprintf(&ptr, "libyaml: parser error: '%s' at line %d, column %d.", ctx->parser.problem, ctx->parser.problem_mark.line, ctx->parser.problem_mark.column);
			break;
			
		case YAML_COMPOSER_ERROR:
			asprintf(&ptr, "libyaml: composer error: '%s' at line %d, column %d.", ctx->parser.problem, ctx->parser.problem_mark.line, ctx->parser.problem_mark.column);
			break;
			
		case YAML_WRITER_ERROR:
			asprintf(&ptr, "libyaml: writer error: '%s' at line %d, column %d.", ctx->parser.problem, ctx->parser.problem_mark.line, ctx->parser.problem_mark.column);
			break;
			
		case YAML_EMITTER_ERROR:
			asprintf(&ptr, "libyaml: emitter error: '%s' at line %d, column %d.", ctx->parser.problem, ctx->parser.problem_mark.line, ctx->parser.problem_mark.column);
			break;
			
		default:
			asprintf(&ptr, "unknown error code (%i) from libyaml. possible memory corruption?", ctx->parser.error);
			break;
	}
	
	assert(ptr);
	return ptr;
}

static const char *event_to_string(yaml_event_type_t event)
{
	switch (event)
	{
		case YAML_NO_EVENT:
			return "YAML_NO_EVENT";
		case YAML_STREAM_START_EVENT:
			return "YAML_STREAM_START_EVENT";
		case YAML_STREAM_END_EVENT:
			return "YAML_STREAM_END_EVENT";
		case YAML_DOCUMENT_START_EVENT:
			return "YAML_DOCUMENT_START_EVENT";
		case YAML_DOCUMENT_END_EVENT:
			return "YAML_DOCUMENT_END_EVENT";
		case YAML_ALIAS_EVENT:
			return "YAML_ALIAS_EVENT";
		case YAML_SCALAR_EVENT:
			return "YAML_SCALAR_EVENT";
		case YAML_SEQUENCE_START_EVENT:
			return "YAML_SEQUENCE_START_EVENT";
		case YAML_SEQUENCE_END_EVENT:
			return "YAML_SEQUENCE_END_EVENT";
		case YAML_MAPPING_START_EVENT:
			return "YAML_MAPPING_START_EVENT";
		case YAML_MAPPING_END_EVENT:
			return "YAML_MAPPING_END_EVENT";
		default:
			return "UNKNOWN";
	}
	
	assert(0);
	return "UNKNOWN";
}

static int is_error_set(parser_context *ctx)
{
	return ctx->error->problem != NULL;
}

static int set_error(parser_context *ctx)
{
	if (!is_error_set(ctx) && ctx->parser.error)
	{
		ctx->error->problem = format_error_string(ctx);
		return 1;
	}
	
	return 0;
}

static int process_event(parser_context *ctx) 
{
	memcpy(&ctx->event, &ctx->next_event, sizeof(yaml_event_t));
	yaml_parser_parse(&ctx->parser, &ctx->next_event);
	return set_error(ctx) ? (-1) : (0);
}

static yaml_event_type_t peek_next_event(parser_context *ctx) 
{
	// just peek the next event
	return ctx->next_event.type;
}

static yaml_event_type_t next_event(parser_context *ctx) 
{
	// process event and return it
	if (process_event(ctx) < 0)
		return YAML_NO_EVENT;
	
	return ctx->event.type;
}

static yaml_node *process_scalar(parser_context *ctx) 
{
	yaml_node *scalar = malloc(sizeof(yaml_node));
	scalar->type = NODE_SCALAR;
	scalar->position.line = ctx->event.start_mark.line;
	scalar->position.column = ctx->event.start_mark.column;
	scalar->data.scalar.value = strdup(ctx->event.data.scalar.value);
	scalar->data.scalar.len = ctx->event.data.scalar.length;
	return scalar;
}

static yaml_node *process_sequence(parser_context *ctx) 
{
	yaml_node *sequence = malloc(sizeof(yaml_node));
	yaml_sequence *seq = &sequence->data.sequence;
	
	sequence->type = NODE_SEQUENCE;
	sequence->position.line = ctx->event.start_mark.line;
	sequence->position.column = ctx->event.start_mark.column;
	
	// zero out values
	seq->count = 0;
	seq->nodes = NULL;
	
	while (peek_next_event(ctx) != YAML_SEQUENCE_END_EVENT)
	{
		yaml_node *node = process_node(ctx);
		
		if (!node)
			return NULL;
		
		// extend space
		seq->nodes = realloc(seq->nodes, (seq->count+1)*sizeof(yaml_node*));
		seq->nodes[seq->count++] = node;
	}
	
	if (process_event(ctx) < 0)
		return NULL;
	
	return sequence;
}

static yaml_node *process_mapping(parser_context *ctx)
{
	yaml_node *mapping = malloc(sizeof(yaml_node));
	yaml_mapping *map = &mapping->data.mapping;
	
	mapping->type = NODE_MAPPING;
	mapping->position.line = ctx->event.start_mark.line;
	mapping->position.column = ctx->event.start_mark.column;
	
	// zero out values
	map->count = 0;
	map->pairs = NULL;
	
	while (peek_next_event(ctx) != YAML_MAPPING_END_EVENT)
	{
		yaml_node_pair *pair = malloc(sizeof(yaml_node_pair));
		
		pair->lhs = process_node(ctx);
		
		if (!pair->lhs)
			return NULL;
		
		pair->rhs = process_node(ctx);
		
		if (!pair->rhs)
			return NULL;
		
		// extend space as needed
		map->pairs = realloc(map->pairs, (map->count+1)*sizeof(yaml_node_pair*));
		map->pairs[map->count++] = pair;
	}
	
	if (process_event(ctx) < 0)
		return NULL;
	
	return mapping;
}

static yaml_node *process_node(parser_context *ctx)
{
	// we expect either: alias, scalar, sequence or mapping
	switch (next_event(ctx)) 
	{
		case YAML_ALIAS_EVENT:
			// TODO: we dont support aliases for now
			asprintf(&ctx->error->problem, "yamltree: there is no support for aliases implemented.");
			return NULL;
			
		case YAML_SCALAR_EVENT:
			return process_scalar(ctx);
		
		case YAML_SEQUENCE_START_EVENT:
			return process_sequence(ctx);
			
		case YAML_MAPPING_START_EVENT:
			return process_mapping(ctx);
			
		default:
			// probably an error
			break;
	}
	
	return NULL;
}

static yaml_document *process_document(parser_context *ctx)
{
	// look for document start event.
	if (next_event(ctx) != YAML_DOCUMENT_START_EVENT)
	{
		if (!is_error_set(ctx))
		{
			asprintf(&ctx->error->problem, "yamltree: expecting YAML_DOCUMENT_START_EVENT got '%s'.", event_to_string(ctx->event.type));
		}
		
		return NULL;
	}
	
	// a document is basically a fancy name for a root node
	yaml_document *doc = process_node(ctx);
	
	if (!doc)
		return NULL;
	
	// get end of document
	if (next_event(ctx) != YAML_DOCUMENT_END_EVENT)
	{
		if (!is_error_set(ctx))
		{
			asprintf(&ctx->error->problem, "yamltree: expecting YAML_DOCUMENT_END_EVENT got '%s'.", event_to_string(ctx->event.type));
		}
		
		return NULL;
	}
	
	return doc;
}

yaml_tree *parse_yaml_stream(FILE *input, yaml_error *error)
{
	parser_context ctx;
	ctx.error = error;
	yaml_parser_initialize(&ctx.parser);
	yaml_parser_set_input_file(&ctx.parser, input);
	
	if (process_event(&ctx) < 0)
		goto error;
	
	if (ctx.next_event.type != YAML_STREAM_START_EVENT)
	{
		asprintf(&ctx.error->problem, "yamltree: expecting YAML_STREAM_START_EVENT got '%s'.", event_to_string(ctx.next_event.type));
		goto error;
	}
	
	yaml_tree *stream = malloc(sizeof(yaml_tree));
	
	stream->count = 0;
	stream->docs = NULL;
	
	while (next_event(&ctx) != YAML_STREAM_END_EVENT)
	{
		// check error
		if (is_error_set(&ctx))
		{
			goto error;
		}
		
		yaml_document *document = process_document(&ctx);
		
		if (!document)
		{
			// TODO: clean up structure
			goto error;
		}
		
		stream->docs = realloc(stream->docs, (stream->count+1)*sizeof(yaml_tree));
		stream->docs[stream->count++] = document;
	}
	
	yaml_parser_delete(&ctx.parser);
	return stream;
	
error:
	yaml_parser_delete(&ctx.parser);
	return NULL;
}

void free_yaml_tree(yaml_tree *tree)
{
	// TODO: implement
}

const char *node_type_str(yaml_node *node)
{
	switch (node->type)
	{
		case NODE_MAPPING:
			return "mapping";
		case NODE_SEQUENCE:
			return "sequence";
		case NODE_SCALAR:
			return "scalar";
		default:
			assert(0);
			break;
	}
	
	assert(0);
	return NULL;
}
