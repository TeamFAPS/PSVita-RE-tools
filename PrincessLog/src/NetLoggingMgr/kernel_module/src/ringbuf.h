#ifndef RINGBUF_H
#define RINGBUF_H

int ringbuf_init(int size);
int ringbuf_term(void);

int ringbuf_put(char *c, int size);
int ringbuf_put_clobber(char *c, int size);
int ringbuf_get(char *c, int size);
int ringbuf_get_wait(char *c, int size);

#endif
