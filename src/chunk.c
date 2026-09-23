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

void expand_chunk(Chunk *chunk)
{
    if (!chunk->expands)
    {
        printf("This chunk cannot be expanded!\n");
        exit(1);
    }
    Chunk new_chunk = {
        .data = malloc(chunk->unit * chunk->max * 2),
        .unit = chunk->unit,
        .max = chunk->max * 2,
        .used = chunk->used,
        .expands = chunk->expands,
    };
    memcpy(new_chunk.data, chunk->data, chunk->unit * chunk->used);
    free(chunk->data);
    *chunk = new_chunk;
}

void *append_data(void *source, int length, Chunk *chunk)
{
    if (chunk->used + length > chunk->max)
        expand_chunk(chunk);
    memcpy(chunk->data + chunk->unit * chunk->used, source, chunk->unit * length);
    void *return_val = chunk->data + chunk->unit * chunk->used;
    chunk->used += length;
    return return_val;
}

void *alloc_mem(int length, Chunk *chunk)
{
    void *return_val = chunk->data + chunk->unit * chunk->used;
    chunk->used += length;
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