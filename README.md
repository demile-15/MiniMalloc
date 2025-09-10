# MiniMalloc
MiniMalloc is a lightweight re-implementation of the C’s `malloc` suite. It manages memory using a free list and grows the heap on demand.

Key features:
- **8-byte alignment:** every block is rounded up to 8 bytes for safety and speed.
- **Guard blocks:** Prologue and epilogue markers protect the heap’s boundaries.
- **Free list management:** uses a circular list to track free blocks, with support for splitting and merging (coalescing).

## Features

### `mini_init()`
- Sets up the memory manager.
- Must be called once at the very start.

### `mini_malloc(size)`
- Reserves a dynamic chunk of memory.
- Grows the heap if needed and always aligns memory properly.

### `mini_free(ptr)`
- Releases a chunk of memory back to the manager.
- Helps recycle space so it can be reused later.

### `mini_realloc(ptr, size)`
- Changes the size of a previously reserved chunk.
- Can shrink, grow in place, or move the data to a new spot if necessary.

## Build & Run
```bash
make        # builds lib and tests
make test   # runs tests with basic assertions
make clean