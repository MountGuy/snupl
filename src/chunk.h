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

void write_data(void *source, int idx, Chunk *chunk);
void write_last(void *source, Chunk *chunk);
void read_data(void *dest, int idx, Chunk *chunk);
void read_last(void *dest, Chunk *chunk);

int has_space(int length, Chunk *chunk);

void *fix_chunk(Chunk *chunk);
void del_chunk(Chunk *chunk);

#endif
