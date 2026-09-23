#ifndef CHUNK_H
#define CHUNK_H 1

#include <stddef.h>

typedef struct {
    void *data;
    size_t unit;
    int max, used, expands;
} Chunk;

Chunk init_chunk(size_t unit, int expands);
void expand_chunk(Chunk *chunk);

void *append_data(void *source, int length, Chunk *chunk);
void *alloc_mem(int length, Chunk *chunk);

void *fix_chunk(Chunk *chunk);
void del_chunk(Chunk *chunk);

#endif
