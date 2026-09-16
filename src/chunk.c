#include "chunk.h"

Chunk init_chunk(size_t unit, int length, int expands)
{
    Chunk chunk = {
        .buf = malloc(unit * length),
        .unit = unit,
        .length = length,
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
        .buf = malloc(chunk->unit * chunk->length * 2),
        .unit = chunk->unit,
        .length = chunk->length * 2,
        .used = chunk->used,
        .expands = chunk->expands,
    };
    memcpy(new_chunk.buf, chunk->buf, chunk->unit * chunk->used);
    free(chunk->buf);
    *chunk = new_chunk;
}

void *append_data(void *source, int length, Chunk *chunk)
{
    if (chunk->used + length > chunk->length)
        expand_chunk(chunk);
    memcpy(chunk->buf + chunk->unit * chunk->used, source, chunk->unit * length);
    void *return_val = chunk->buf + chunk->unit * chunk->used;
    chunk->used += length;
    return return_val;
}

void *alloc_mem(int length, Chunk *chunk)
{
    void *return_val = chunk->buf + chunk->unit * chunk->used;
    chunk->used += length;
    return return_val;
}

void write_data(void *source, int idx, Chunk *chunk)
{
    memcpy(chunk->buf + chunk->unit * idx, source, chunk->unit);
}

void write_last(void *source, Chunk *chunk)
{
    write_data(source, chunk->used - 1, chunk);
}

void read_data(void *dest, int idx, Chunk *chunk)
{
    memcpy(dest, chunk->buf + chunk->unit * idx, chunk->unit);
}

void read_last(void *dest, Chunk *chunk)
{
    read_data(dest, chunk->used - 1, chunk);
}

int has_space(int length, Chunk *chunk)
{
    return chunk->length >= chunk->used + length;
}