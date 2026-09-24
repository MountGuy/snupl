#include "chunk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEF_MAX 1000

Chunk init_chunk(size_t unit, int expands)
{
    Chunk chunk = {
        .data = malloc(unit * DEF_MAX),
        .unit = unit,
        .max = DEF_MAX,
        .used = 0,
        .expands = expands,
    };

    return chunk;
}

Chunk init_large_chunk(size_t unit, int size, int expands)
{
    if (DEF_MAX > size)
        size = DEF_MAX;
    Chunk chunk = {
        .data = malloc(unit * size),
        .unit = unit,
        .max = size,
        .used = 0,
        .expands = expands,
    };

    return chunk;
}

void expand_chunk(Chunk *chunk)
{
    if (!chunk->expands)
    {
        printf("This chunk cannot be expanded!\n");
        exit(1);
    }
    
    void *data = chunk->data;
    chunk->data = malloc(chunk->unit * (chunk->max *= 2));
    memcpy(chunk->data, data, chunk->unit * chunk->used);
    free(data);
}

void *alloc_mem(int length, Chunk *chunk)
{
    void *return_val = chunk->data + chunk->unit * chunk->used;
    chunk->used += length;

    return return_val;
}

void *append_data(void *source, int length, Chunk *chunk)
{
    if (chunk->used + length > chunk->max)
        expand_chunk(chunk);

    void *return_val = alloc_mem(length, chunk);
    memcpy(return_val, source, chunk->unit * length);

    return return_val;
}

void *fix_chunk(Chunk *chunk)
{
    int size = chunk->unit * chunk->used;
    void *data = malloc(size);
    memcpy(data, chunk->data, size);
    free(chunk->data);

    return data;
}

void del_chunk(Chunk *chunk)
{
    free(chunk->data);
}