#include "file.h"

int fsize(FILE *fp) {
    int prev = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
}

/* Read the contents of a file into a buffer.  Return the size of the file 
 * and set buf to point to a buffer allocated with malloc that contains  
 * the file contents.
 */
int read_file(FILE *fp, char **buf) {
  int n, np, r;
  char *b, *b2;

  n = 1;
  np = n;
  b = malloc(sizeof(char)*n);
  while ((r = fread(b, sizeof(char), 1, fp)) > 0) {
    n += r;
    if (np - n < 1) { 
      np *= 2;                      // buffer is too small, the next read could overflow!
      b2 = malloc(np*sizeof(char));
      memcpy(b2, b, n * sizeof(char));
      free(b);
      b = b2;
    }
  }
  *buf = b;
  rewind(fp);
  return n - 1;
}

int read_byte(FILE *fp, int offset, char **buf) {
	int prev = ftell(fp);
    fseek(fp, offset, SEEK_SET);
	int result = -1;
	char *b;
	int n = 1;
	b = malloc(sizeof(char)*n);
	fread(b, sizeof(char), n, fp);
	if (b[0] < 0) result = b[0] + 0x100;
	else result = b[0];
	*buf = b;
	fseek(fp, prev, SEEK_SET);// go back to where we were
	return result;
}