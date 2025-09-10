#include <string.h>
#include "mini_helpers.h"


block_t *prologue;
block_t *epilogue;
block_t *flist_first;


// ============== Local helpers ==============
// Split a block into [first_size | second_size]
static inline void split(block_t *first, long first_sz, int first_alloc,
                         long second_sz, int second_alloc){
    block_set_size_and_allocated(first, first_sz, first_alloc);
    block_t *second = block_next(first);
    block_set_size_and_allocated(second, second_sz, second_alloc);
}


// Shrink a block in place if possible
static void *shrink_block(block_t *b, long orig, long want) {
    block_t *n = block_next(b);
    int n_alloc = block_allocated(n);
    long leftover = orig - want;

    if (leftover == 0 || (n_alloc && leftover < MINBLOCKSIZE)) return b->payload;

    split(b, want, 1, leftover, 0);
    block_t *free_part = block_next(b);
    if (!coalesce(free_part)) insert_free_block(free_part);
    return b->payload;
}


// Expand a block into the next free block
static void *expand_block(block_t *b, block_t *n, long want, long total) {
    pull_free_block(n);
    block_set_size_and_allocated(b, total, 1);
    long leftover = total - want;
    if (leftover >= MINBLOCKSIZE) {
        split(b, want, 1, leftover, 0);
        insert_free_block(block_next(b));
    }
    return b->payload;
}


// Try to coalesce `free_block` with neighbors; returns 1 if coalesced
static int coalesce(block_t *free_block) {
    block_t *next = block_next(free_block);
    block_t *prev = block_prev(free_block);

    int next_free = !block_allocated(next);
    int prev_free = !block_allocated(prev);
    long new_size = block_size(free_block);

    if (!next_free && !prev_free) return 0;

    if (next_free) {
        pull_free_block(next);
        new_size += block_size(next);
        block_set_size_and_allocated(free_block, new_size, 0);
    }

    if (prev_free) {
        pull_free_block(prev);
        new_size += block_size(prev);
        block_set_size_and_allocated(prev, new_size, 0);
        free_block = prev;
    }

    insert_free_block(free_block);
    return 1;
}


// Extend the heap by at least `size`; returns payload of new allocated block
static void *heap_extend(long size) {
    block_t *prev = block_prev(epilogue);
    long extend_size = size;

    // if the block before epilogue is free, consume it first
    if (!block_allocated(prev)) {
        long prev_size = block_size(prev);
        if (prev_size < size) {
            pull_free_block(prev);
            extend_size -= prev_size;
            epilogue = prev; // repurpose prev as the start of the new chunk
        } else {
            // If prev already large enough, just allocate from it
            pull_free_block(prev);
            block_set_size_and_allocated(prev, size, 1);
            // Put leftover back as free if big enough
            long leftover = prev_size - size;
            if (leftover >= MINBLOCKSIZE) {
                split(prev, size, 1, leftover, 0);
                insert_free_block(block_next(prev));
            }
            return prev->payload;
        }
    }

    if (mem_sbrk(extend_size) == (void *)-1) return NULL;
    // The contiguous region [epilogue .. epilogue+extend_size) now exists
    block_set_size_and_allocated(epilogue, size, 1);
    epilogue = block_next(epilogue);
    block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);
    return block_prev(epilogue)->payload;
}


// ===== Public API =====
int mini_init(void) {
    // Allocate prologue and epilogue
    if ((prologue = mem_sbrk(TAGS_SIZE)) == (void *)-1) return -1;
    if ((epilogue = mem_sbrk(TAGS_SIZE)) == (void *)-1) return -1;
    block_set_size_and_allocated(prologue, TAGS_SIZE, 1);
    block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);
    flist_first = NULL;

    // Start heap with an initial free block (~512 total bytes)
    void *p = mini_malloc(512 - TAGS_SIZE);
    if (!p) return -1;
    mini_free(p);
    return 0;
}


void *mini_malloc(long payload_size) {
    long req = get_valid_size(payload_size);
    if (req == -1) return NULL;

    // First-fit search over explicit free list
    block_t *cur = flist_first;
    if (cur) {
        do {
            long avail = block_size(cur);
            if (avail >= req) {
                long leftover = avail - req;
                pull_free_block(cur);
                if (leftover >= MINBLOCKSIZE) {
                    split(cur, req, 1, leftover, 0);
                    insert_free_block(block_next(cur));
                } else {
                    block_set_size_and_allocated(cur, avail, 1);
                }
                return cur->payload;
            }
            cur = (block_t *)cur->payload[0]; // flink
        } while (cur && cur != flist_first);
    }

    // No fit: extend heap
    return heap_extend(req);
}


void mini_free(void *ptr) {
    if (!ptr) return;
    block_t *b = payload_to_block(ptr);
    block_set_allocated(b, 0);
    if (!coalesce(b)) insert_free_block(b);
}


void *mini_realloc(void *ptr, long new_payload_size) {
    // Handle malloc/free semantics first
    if (ptr == NULL) return mini_malloc(new_payload_size);
    if (new_payload_size == 0) { mini_free(ptr); return NULL; }

    // Normalize the requested size (payload -> total block size)
    long want = get_valid_size(new_payload_size);
    if (want == -1) return NULL;  // invalid request

    // Current block and sizes
    block_t *b   = payload_to_block(ptr);
    long     bsz = block_size(b);               // current total block size

    // 1) If shrinking fits in-place, do it and return early
    if (want <= bsz) {
        return shrink_block(b, bsz, want);
    }

    // 2) Try to grow in-place by merging with the next free block
    block_t *n       = block_next(b);
    int      n_alloc = block_allocated(n);
    long     nsz     = block_size(n);
    if (!n_alloc) {
        long total = bsz + nsz;
        if (want <= total) {
            return expand_block(b, n, want, total);
        }
    }

    // 3) Fallback: allocate a new block, copy min(old_payload, new_payload), free old
    void *np = mini_malloc(new_payload_size);
    if (!np) return NULL;

    long old_payload = bsz  - TAGS_SIZE;
    long new_payload = want - TAGS_SIZE;
    size_t bytes_to_copy = (size_t)(old_payload < new_payload ? old_payload : new_payload);

    memcpy(np, ptr, bytes_to_copy);
    mini_free(ptr);
    return np;
}
