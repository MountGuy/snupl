#include <stdlib.h>
#include <string.h>

#define DEF_MAX 1000
#define C_NULL ('\0')

typedef double MetaExpr;

char *character_s = "character", *chunk_s = "chunk", *pointer_s = "pointer";

typedef struct {
    void *buf;
    size_t unit;
    int length, used, expands;
} Chunk;

typedef struct {
    Chunk strings, string_heads, exprs, expr_lists;
    int string_num;
} Arena;

void print_chunk(Chunk *chunk)
{
    printf("chunk pointer: %p\nunit: %d\nused_num: %d\nused: %d\n\n", chunk->buf, (int)chunk->unit, chunk->used, chunk->used);
}

Chunk init_chunk(size_t unit, int length)
{
    Chunk chunk = {
        .buf = malloc(unit * length),
        .unit = unit,
        .length = length,
        .used = 0,
        .expands = 0,
    };

    return chunk;
}

void expand_chunk(Chunk *chunk)
{
    if (!chunk->expands)
    {
        printf("This chunk cannot expand!\n");
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

void init_arena(Arena *arena)
{
    Chunk chunk;
    arena->strings = init_chunk(sizeof(Chunk), DEF_MAX);
    chunk = init_chunk(sizeof(char), DEF_MAX);
    append_data(&chunk, 1, &arena->strings);

    arena->string_heads = init_chunk(sizeof(void*), DEF_MAX);
    arena->string_num = 0;

    arena->exprs = init_chunk(sizeof(Chunk), DEF_MAX);
    chunk = init_chunk(sizeof(MetaExpr), DEF_MAX);
    append_data(&chunk, 1, &arena->exprs);

    arena->expr_lists = init_chunk(sizeof(Chunk), DEF_MAX);
    chunk = init_chunk(sizeof(MetaExpr*), DEF_MAX);
    append_data(&chunk, 1, &arena->exprs);
}

char *insert_string(char *string, int string_len, Arena *arena)
{
    int alloc_len = string_len + 1 > DEF_MAX? string_len + 1 : DEF_MAX;
    Chunk last_buf;
    read_last(&last_buf, &arena->strings);

    if (!has_space(string_len + 1, &last_buf))
    {
        if (!has_space(1, &arena->strings))
            expand_chunk(&arena->strings);

        last_buf = init_chunk(sizeof(char), alloc_len);
        append_data(&last_buf, 1, &arena->strings);
    }

    char *head = append_data(string, string_len, &last_buf);
    char null = C_NULL;
    append_data(&null, 1, &last_buf);
    write_last(&last_buf, &arena->strings);

    return head;
}

void insert_string_head(char *string_head, Arena *arena)
{
    if (!has_space(1, &arena->string_heads))
        expand_chunk(&arena->string_heads);
    append_data(&string_head, 1, &arena->string_heads);
}

char *add_string(char *string, int string_len, Arena *arena)
{
    char *str;
    for (int i = 0; i < arena->string_num; i++)
    {
        read_data(&str, i, &arena->string_heads);
        if (strncmp(str, string, string_len) == 0 && strlen(str) == string_len)
            return str;
    }
    
    str = insert_string(string, string_len, arena);
    insert_string_head(str, arena);
    arena->string_num++;

    return str;
}

MetaExpr *alloc_expr(Arena *arena)
{
    Chunk last_buf;
    read_last(&last_buf, &arena->exprs);

    if (!has_space(1, &last_buf))
    {
        if (!has_space(1, &arena->exprs))
            expand_chunk(&arena->exprs);
        last_buf = init_chunk(sizeof(MetaExpr), DEF_MAX);
        append_data(&last_buf, 1, &arena->exprs);
    }

    MetaExpr *head = alloc_mem(1, &last_buf);
    write_last(&last_buf, &arena->exprs);

    return head;
}

MetaExpr **alloc_exprs(int size, Arena *arena)
{
    int alloc_size = size + 1 > DEF_MAX? size + 1 : DEF_MAX;
    Chunk last_buf;
    read_last(&last_buf, &arena->expr_lists);

    if (!has_space(alloc_size, &last_buf))
    {
        if (!has_space(1, &arena->expr_lists))
            expand_chunk(&arena->expr_lists);
        last_buf = init_chunk(sizeof(MetaExpr*), DEF_MAX);
        append_data(&last_buf, 1, &arena->expr_lists);
    }

    MetaExpr **head = alloc_mem(alloc_size, &last_buf);
    write_last(&last_buf, &arena->expr_lists);

    return head;

}
