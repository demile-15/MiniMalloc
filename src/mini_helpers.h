#ifndef MINI_HELPERS_H_
#define MINI_HELPERS_H_

#include <assert.h>
#include <stddef.h>

// Alignment & size constants
#define ALIGNMENT 8
#define WORD_SIZE ((long)sizeof(long))
#define TAGS_SIZE (2 * WORD_SIZE)
#define MINBLOCKSIZE (4 * WORD_SIZE)

// Heap block layout
typedef struct block {
    long size;        // total size; LSB=1 if allocated
    long payload[];   // if free: payload[0]=flink, payload[1]=blink
} block_t;

// Globals defined in mini_core.c
extern block_t *prologue;
extern block_t *epilogue;
extern block_t *flist_first;

// ===== Helper API (implemented in mini_helpers.c) =====
long align8(long x);
long valid_req_size(long payload);

long *block_end_tag(block_t *b);
int   block_allocated(block_t *b);
long  block_size(block_t *b);
void  block_set_size(block_t *b, long size);
void  block_set_allocated(block_t *b, int allocated);
void  block_set_size_and_allocated(block_t *b, long size, int alloc);

block_t *block_prev(block_t *b);
block_t *block_next(block_t *b);
block_t *payload_to_block(void *payload);

block_t *block_flink(block_t *b);
block_t *block_blink(block_t *b);
void     block_set_flink(block_t *b, block_t *n);
void     block_set_blink(block_t *b, block_t *p);

void pull_free_block(block_t *fb);
void insert_free_block(block_t *fb);

#endif // MINI_HELPERS_H_
