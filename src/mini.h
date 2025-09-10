#ifndef MINI_H_
#define MINI_H_

#include <stddef.h>

int   mini_init(void);
void *mini_malloc(long size);
void  mini_free(void *ptr);
void *mini_realloc(void *ptr, long size);

#endif // MINI_H_