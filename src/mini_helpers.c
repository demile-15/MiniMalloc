#include "mini_helpers.h"

block_t *prologue;
block_t *epilogue;
block_t *flist_first;

long align8(long x) {
    return (x + (WORD_SIZE - 1)) & ~(WORD_SIZE - 1);
}

long valid_req_size(long payload) {
    if (payload <= 0) return -1;
    long sz = align8(payload + TAGS_SIZE);
    if (sz < MINBLOCKSIZE) sz = MINBLOCKSIZE;
    return sz;
}

long *block_end_tag(block_t *b) {
    return &b->payload[b->size / 8 - 2];
}

int block_allocated(block_t *b) {
    return b->size & 1;
}

long block_size(block_t *b) {
    return b->size & -2;
}

void block_set_size(block_t *b, long size) {
    size |= block_allocated(b);
    b->size = size;
    *block_end_tag(b) = size;
}

void block_set_allocated(block_t *b, int allocated) {
    if (allocated) {
        b->size |= 1;
        *block_end_tag(b) |= 1;
    } else {
        b->size &= -2;
        *block_end_tag(b) &= -2;
    }
}

void block_set_size_and_allocated(block_t *b, long size, int alloc) {
    block_set_size(b, size);
    block_set_allocated(b, alloc);
}

block_t *block_prev(block_t *b) {
    return (block_t *)((char *)b - (b->payload[-2] & -2));
}

block_t *block_next(block_t *b) {
    return (block_t *)((char *)b + (b->size & -2));
}

block_t *payload_to_block(void *payload) {
    return (block_t *)((long *)payload - 1);
}

block_t *block_flink(block_t *b) { return (block_t *)b->payload[0]; }
block_t *block_blink(block_t *b) { return (block_t *)b->payload[1]; }

void block_set_flink(block_t *b, block_t *n) { b->payload[0] = (long)n; }
void block_set_blink(block_t *b, block_t *p) { b->payload[1] = (long)p; }

void pull_free_block(block_t *fb) {
    block_t *pr = block_blink(fb), *nx = block_flink(fb);
    if (pr == fb && nx == fb) {
        flist_first = NULL;
    } else {
        if (flist_first == fb) flist_first = nx;
        block_set_flink(pr, nx);
        block_set_blink(nx, pr);
    }
}

void insert_free_block(block_t *fb) {
    if (flist_first) {
        block_t *last = block_blink(flist_first);
        block_set_flink(fb, flist_first);
        block_set_blink(fb, last);
        block_set_flink(last, fb);
        block_set_blink(flist_first, fb);
    } else {
        block_set_blink(fb, fb);
        block_set_flink(fb, fb);
    }
    flist_first = fb;
}
