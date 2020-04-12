#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "unarzl.h"


int main(int argc, const char *argv[]) {
  int ret = 1;
  FILE *input = NULL;
  FILE *output= NULL;
  uint32_t size = 0;
  uint32_t count = 0;
  uint8_t *inbuf = NULL;
  uint8_t *outbuf = NULL;

  if (argc < 2) {
    fprintf(stderr, "usage: %s input [output]\n", argv[0]);
    return 1;
  }
  if ((input = fopen(argv[1], "rb")) == NULL) {
    perror("input");
    goto error;
  }
  if (argc > 2) {
    if ((output = fopen(argv[2], "wb")) == NULL) {
      perror("output");
      goto error;
    }
  } else if ((output = fopen("uncompressed_data.bin", "wb")) == NULL) {
    perror("output");
    goto error;
  }
  fseek(input, 0, SEEK_END);
  if ((size = ftell(input)) < 0) {
    perror("input");
    goto error;
  }
  fseek(input, 0, SEEK_SET);
  if ((inbuf = malloc(size)) == NULL) {
    perror("memory");
    goto error;
  }
  count = 0;
  while ((count = fread(inbuf+count, sizeof(uint8_t), size, input)) < size) {
    if (count < 0) {
      perror("read");
      goto error;
    }
    size -= count;
  }

  outbuf = unarzl(inbuf, &size);

  count = 0;
  while ((count = fwrite(outbuf+count, sizeof(uint8_t), size, output)) < size) {
    if (count < 0) {
      perror("write");
      goto error;
    }
    size -= count;
  }
  ret = 0;
  
error:
  free(outbuf);
  free(inbuf);
  fclose(output);
  fclose(input);
  
  return ret;
}