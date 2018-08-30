#ifndef VITA_IMPORT_H
#define VITA_IMPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* These fields must always come at the beginning of the NID-bearing structs */
typedef struct {
	char *name;
	uint32_t NID;
} vita_imports_common_fields;

typedef struct {
	char *name;
	uint32_t NID;
} vita_imports_stub_t;

typedef struct {
	char *name;
	uint32_t NID;
	bool is_kernel;
	vita_imports_stub_t **functions;
	vita_imports_stub_t **variables;
	int n_functions;
	int n_variables;
	uint32_t flags;
} vita_imports_module_t;

typedef struct {
	char *name;
	uint32_t NID;
	vita_imports_module_t **modules;
	int n_modules;
} vita_imports_lib_t;

typedef struct {
	vita_imports_lib_t **libs;
	int n_libs;
} vita_imports_t;


vita_imports_t *vita_imports_load(const char *filename, int verbose);
vita_imports_t *vita_imports_loads(FILE *text, int verbose);

vita_imports_t *vita_imports_new(int n_libs);
void vita_imports_free(vita_imports_t *imp);

vita_imports_lib_t *vita_imports_find_lib(vita_imports_t *imp, uint32_t NID);


vita_imports_lib_t *vita_imports_lib_new(const char *name, uint32_t NID, int n_modules);
void vita_imports_lib_free(vita_imports_lib_t *lib);

vita_imports_module_t *vita_imports_find_module(vita_imports_lib_t *lib, uint32_t NID);


vita_imports_module_t *vita_imports_module_new(const char *name, bool kernel, uint32_t NID, int n_functions, int n_variables);
void vita_imports_module_free(vita_imports_module_t *mod);

vita_imports_stub_t *vita_imports_find_function(vita_imports_module_t *mod, uint32_t NID);
vita_imports_stub_t *vita_imports_find_variable(vita_imports_module_t *mod, uint32_t NID);


vita_imports_stub_t *vita_imports_stub_new(const char *name, uint32_t NID);
void vita_imports_stub_free(vita_imports_stub_t *stub);

#ifdef __cplusplus
}
#endif

#endif
