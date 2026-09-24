#include "arena.h"

#define LAST_DATA(c) (((c)->data) + ((c)->used - 1) * ((c)->unit))
#define HAS_SPACE(c, s) ((c)->max >= (c)->used + (s))

Arena init_arena()
{
    Arena arena;
    Chunk chunk;

    arena.strings = init_chunk(sizeof(Chunk), true);
    chunk = init_chunk(sizeof(char), false);
    append_data(&chunk, 1, &arena.strings);

    arena.string_heads = init_chunk(sizeof(void*), true);
    arena.string_lens = init_chunk(sizeof(int), true);
    arena.string_num = 0;

    arena.mexprs = init_chunk(sizeof(Chunk), true);
    chunk = init_chunk(sizeof(MetaExpr), false);
    append_data(&chunk, 1, &arena.mexprs);

    arena.exprs = init_chunk(sizeof(Chunk), true);
    chunk = init_chunk(sizeof(Expr), false);
    append_data(&chunk, 1, &arena.exprs);

    return arena;
}

char *insert_string(char *string, int string_len, Arena *arena)
{
    Chunk *last_chunk = LAST_DATA(&arena->strings);

    if (!HAS_SPACE(last_chunk, string_len + 1))
    {
        Chunk new_chunk = init_large_chunk(sizeof(char), string_len + 1, false);
        append_data(&new_chunk, 1, &arena->strings);
        last_chunk = LAST_DATA(&arena->strings);
    }

    char *head = append_data(string, string_len, last_chunk);
    char null = c_null;
    append_data(&null, 1, last_chunk);

    return head;
}

char *add_string(char *string, int string_len, Arena *arena)
{
    char **strs = arena->string_heads.data;
    int *lens = arena->string_lens.data;

    for (int i = 0; i < arena->string_num; i++)
        if (strncmp(strs[i], string, string_len) == 0 && lens[i] == string_len)
            return strs[i];
   
    char *str = insert_string(string, string_len, arena);
    append_data(&str, 1, &arena->string_heads);
    append_data(&string_len, 1, &arena->string_lens);
    arena->string_num++;

    return str;
}

MetaExpr *alloc_mexpr(int size, Arena *arena)
{
    Chunk *last_chunk = LAST_DATA(&arena->mexprs);

    if (!HAS_SPACE(last_chunk, size))
    {
        Chunk new_chunk = init_large_chunk(sizeof(MetaExpr), size, false);
        append_data(&new_chunk, 1, &arena->mexprs);
        last_chunk = LAST_DATA(&arena->mexprs);
    }

    return alloc_mem(size, last_chunk);
}

Expr *alloc_expr(int size, Arena *arena)
{
    Chunk *last_chunk = LAST_DATA(&arena->exprs);

    if (!HAS_SPACE(last_chunk, size))
    {
        Chunk new_chunk = init_large_chunk(sizeof(Expr), size, false);
        append_data(&new_chunk, 1, &arena->exprs);
        last_chunk = LAST_DATA(&arena->exprs);
    }

    return alloc_mem(size, last_chunk);
}
