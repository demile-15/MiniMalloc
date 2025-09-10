#include <string.h>
#include "mini_helpers.h"


block_t *prologue;
block_t *epilogue;
block_t *flist_first;


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