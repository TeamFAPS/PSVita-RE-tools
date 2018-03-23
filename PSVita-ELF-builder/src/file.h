#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>


int fsize(FILE *fp);
int read_file(FILE *fp, char **buf);
int read_byte(FILE *fp, int offset, char **b);

#endif /* FILE_H */