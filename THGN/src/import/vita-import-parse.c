#include <string.h>
#include <stdlib.h>
#include "vita-import.h"
#include "yamltree.h"
#include "yamltreeutil.h"

int process_import_functions(yaml_node *parent, yaml_node *child, vita_imports_module_t *library) {
	if (!is_scalar(parent)) {
		fprintf(stderr, "error: line: %zd, column: %zd, expecting function to be scalar, got '%s'.\n"
			, parent->position.line
			, parent->position.column
			, node_type_str(parent));
		
		return -1;
	}
	
	yaml_scalar *key = &parent->data.scalar;
		
	// create an export symbol for this function
	vita_imports_stub_t *symbol = vita_imports_stub_new(key->value,0);
		
	if (!is_scalar(child)) {
		fprintf(stderr, "error: line: %zd, column: %zd, expecting function value to be scalar, got '%s'.\n"
			, child->position.line
			, child->position.column
			, node_type_str(child));
		
		return -1;
	}
	
	if (process_32bit_integer(child, &symbol->NID) < 0) {
		fprintf(stderr, "error: line: %zd, column: %zd, could not convert function nid '%s' to 32 bit integer.\n", child->position.line, child->position.column, child->data.scalar.value);
		return -1;
	}
	// append to list
	library->functions = realloc(library->functions, (library->n_functions+1)*sizeof(vita_imports_stub_t*));
	library->functions[library->n_functions++] = symbol;
	
	return 0;
}

int process_import_variables(yaml_node *parent, yaml_node *child, vita_imports_module_t *library) {
	if (!is_scalar(parent)) {
		fprintf(stderr, "error: line: %zd, column: %zd, expecting variable to be scalar, got '%s'.\n"
			, parent->position.line
			, parent->position.column
			, node_type_str(parent));
		
		return -1;
	}
	
	yaml_scalar *key = &parent->data.scalar;
		
	// create an export symbol for this variable
	vita_imports_stub_t *symbol = vita_imports_stub_new(key->value,0);
			
	if (!is_scalar(child)) {
		fprintf(stderr, "error: line: %zd, column: %zd, expecting variable value to be scalar, got '%s'.\n"
			, child->position.line
			, child->position.column
			, node_type_str(child));
		
		return -1;
	}
	
	if (process_32bit_integer(child, &symbol->NID) < 0) {
		fprintf(stderr, "error: line: %zd, column: %zd, could not convert variable nid '%s' to 32 bit integer.\n", child->position.line, child->position.column, child->data.scalar.value);
		return -1;
	}
	// append to list
	library->variables = realloc(library->variables, (library->n_variables+1)*sizeof(vita_imports_stub_t*));
	library->variables[library->n_variables++] = symbol;
	
	return 0;
}

int process_library(yaml_node *parent, yaml_node *child, vita_imports_module_t *library) {
	if (!is_scalar(parent)) {
		fprintf(stderr, "error: line: %zd, column: %zd, expecting library key to be scalar, got '%s'.\n", parent->position.line, parent->position.column, node_type_str(parent));
		return -1;
	}
	
	yaml_scalar *key = &parent->data.scalar;
	
	if (strcmp(key->value, "kernel") == 0) {
		if (!is_scalar(child)) {
			fprintf(stderr, "error: line: %zd, column: %zd, expecting library syscall flag to be scalar, got '%s'.\n", child->position.line, child->position.column, node_type_str(child));
			return -1;
		}
		
		if (process_bool(child, &library->is_kernel) < 0) {
			fprintf(stderr, "error: line: %zd, column: %zd, could not convert library flag to boolean, got '%s'. expected 'true' or 'false'.\n", child->position.line, child->position.column, child->data.scalar.value);
			return -1;
		}
	}
	else if (strcmp(key->value, "functions") == 0) {
		if (yaml_iterate_mapping(child, (mapping_functor)process_import_functions, library) < 0)
			return -1;
	}
	else if (strcmp(key->value, "variables") == 0) {
		if (yaml_iterate_mapping(child, (mapping_functor)process_import_variables, library) < 0)
			return -1;
	}
	else if (strcmp(key->value, "nid") == 0) {
		if (!is_scalar(child)) {
			fprintf(stderr, "error: line: %zd, column: %zd, expecting library nid to be scalar, got '%s'.\n", child->position.line, child->position.column, node_type_str(child));
			return -1;
		}
		
		if (process_32bit_integer(child, &library->NID) < 0) {
			fprintf(stderr, "error: line: %zd, column: %zd, could not convert library nid '%s' to 32 bit integer.\n", child->position.line, child->position.column, child->data.scalar.value);
			return -1;
		}
	}
	else {
		fprintf(stderr, "error: line: %zd, column: %zd, unrecognised library key '%s'.\n", child->position.line, child->position.column, key->value);
		return -1;
	}
	
	return 0;
}

int process_libraries(yaml_node *parent, yaml_node *child, vita_imports_lib_t *import) {
	if (!is_scalar(parent)) {
		fprintf(stderr, "error: line: %zd, column: %zd, expecting library key to be scalar, got '%s'.\n", parent->position.line, parent->position.column, node_type_str(parent));
		return -1;
	}
	
	yaml_scalar *key = &parent->data.scalar;
	
	vita_imports_module_t *library = vita_imports_module_new("",false,0,0,0);
	
	// default values
	library->name = strdup(key->value);

	if (yaml_iterate_mapping(child, (mapping_functor)process_library, library) < 0)
		return -1;

	import->modules = realloc(import->modules, (import->n_modules+1)*sizeof(vita_imports_module_t*));
	import->modules[import->n_modules++] = library;
		
	return 0;
}

int process_import(yaml_node *parent, yaml_node *child, vita_imports_lib_t *import) {
	if (!is_scalar(parent)) {
		fprintf(stderr, "error: line: %zd, column: %zd, expecting module key to be scalar, got '%s'.\n", parent->position.line, parent->position.column, node_type_str(parent));
		return -1;
	}
	
	yaml_scalar *key = &parent->data.scalar;

	if (strcmp(key->value, "nid") == 0) {
		if (!is_scalar(child)) {
			fprintf(stderr, "error: line: %zd, column: %zd, expecting module nid to be scalar, got '%s'.\n", child->position.line, child->position.column, node_type_str(child));
			return -1;
		}
		
		if (process_32bit_integer(child, &import->NID) < 0) {
			fprintf(stderr, "error: line: %zd, column: %zd, could not convert module nid '%s' to 32 bit integer.\n", child->position.line, child->position.column, child->data.scalar.value);
			return -1;
		}
	}
	else if (strcmp(key->value, "libraries") == 0) {
			
		if (yaml_iterate_mapping(child, (mapping_functor)process_libraries, import) < 0)
			return -1;

	}
	else {
		fprintf(stderr, "error: line: %zd, column: %zd, unrecognised module key '%s'.\n", child->position.line, child->position.column, key->value);
		return -1;
	}
	
	return 0;
}

int process_import_list(yaml_node *parent, yaml_node *child, vita_imports_t *imports) {
	if (!is_scalar(parent)) {
		fprintf(stderr, "error: line: %zd, column: %zd, expecting modules key to be scalar, got '%s'.\n", parent->position.line, parent->position.column, node_type_str(parent));
		return -1;
	}
	
	yaml_scalar *key = &parent->data.scalar;
	
	vita_imports_lib_t *import = vita_imports_lib_new(key->value,0,0);
	
	if (yaml_iterate_mapping(child, (mapping_functor)process_import, import) < 0)
		return -1;
	
	imports->libs = realloc(imports->libs, (imports->n_libs+1)*sizeof(vita_imports_lib_t*));
	imports->libs[imports->n_libs++] = import;
	return 0;
}

vita_imports_t *read_vita_imports(yaml_document *doc) {
	if (!is_mapping(doc)) {
		fprintf(stderr, "error: line: %zd, column: %zd, expecting root node to be a mapping, got '%s'.\n", doc->position.line, doc->position.column, node_type_str(doc));
		return NULL;
	}
	
	yaml_mapping *root = &doc->data.mapping;
	
	// check we only have one entry
	if (root->count < 1) {
		fprintf(stderr, "error: line: %zd, column: %zd, expecting at least one entry within root mapping, got %zd.\n", doc->position.line, doc->position.column, root->count);
		return NULL;
	}
	
	vita_imports_t *imports  = vita_imports_new(0);

	for(int n = 0; n < root->count; n++){
		// check lhs is a scalar
		if (is_scalar(root->pairs[n]->lhs)) {
		
			if (strcmp(root->pairs[n]->lhs->data.scalar.value, "modules")==0) {
				if (yaml_iterate_mapping(root->pairs[n]->rhs, (mapping_functor)process_import_list, imports) < 0)
					return NULL;
				continue;
			}
			
			// fprintf(stderr, "warning: line: %zd, column: %zd, unknow tag '%s'.\n", root->pairs[n]->lhs->position.line, root->pairs[n]->lhs->position.column, root->pairs[n]->lhs->data.scalar.value);

		}

	}
	
	return imports;
	
}

vita_imports_t *vita_imports_load(const char *filename, int verbose)
{
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		fprintf(stderr, "Error: could not open %s\n", filename);
		return NULL;
	}
	vita_imports_t *imports = vita_imports_loads(fp, verbose);

	fclose(fp);

	return imports;
}

vita_imports_t *vita_imports_loads(FILE *text, int verbose){
	uint32_t nid = 0;
	yaml_error error = {0};
	
	yaml_tree *tree = parse_yaml_stream(text, &error);
	
	if (!tree)
	{
		fprintf(stderr, "error: %s\n", error.problem);
		free(error.problem);
		return NULL;
	}
	
	if (tree->count != 1)
	{
		fprintf(stderr, "error: expecting a single yaml document, got: %zd\n", tree->count);
		// TODO: cleanup tree
		return NULL;
	}
	
	return read_vita_imports(tree->docs[0]);
}
