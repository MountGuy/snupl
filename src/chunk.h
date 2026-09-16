#ifndef CHUNK_H
#define CHUNK_H 1

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "struct.h"

#define DEF_MAX 1000

Chunk init_chunk(size_t unit, int max, int expands);
void expand_chunk(Chunk *chunk);
void *append_data(void *source, int length, Chunk *chunk);
void *alloc_mem(int length, Chunk *chunk);
void write_data(void *source, int idx, Chunk *chunk);
void write_last(void *source, Chunk *chunk);
void read_data(void *dest, int idx, Chunk *chunk);
void read_last(void *dest, Chunk *chunk);

#endif
