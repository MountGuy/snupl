#include "arena.h"

#define LAST_DATA(c) (((c)->data) + ((c)->used - 1) * ((c)->unit))
#define HAS_SPACE(c, s) ((c)->max >= (c)->used + (s))

Chunk init_large_chunk(size_t unit, int expands, int max)
{
    Chunk chunk = init_chunk(unit, true);
    while (chunk.max < max)
        expand_chunk(&chunk);
    chunk.expands = false;

    return chunk;
}

Arena init_arena()
{
    Arena arena;
    Chunk chunk;
    arena.strings = init_chunk(sizeof(Chunk), true);
    chunk = init_chunk(sizeof(char), false);
    append_data(&chunk, 1, &arena.strings);

    arena.string_heads = init_chunk(sizeof(void*), true);
    arena.string_num = 0;

    arena.exprs = init_chunk(sizeof(Chunk), true);
    chunk = init_chunk(sizeof(MetaExpr), false);
    append_data(&chunk, 1, &arena.exprs);

    arena.expr_lists = init_chunk(sizeof(Chunk), true);
    chunk = init_chunk(sizeof(MetaExpr*), false);
    append_data(&chunk, 1, &arena.expr_lists);

    return arena;
}

char *insert_string(char *string, int string_len, Arena *arena)
{
    Chunk *last_chunk = LAST_DATA(&arena->strings);

    if (!HAS_SPACE(last_chunk, string_len + 1))
    {
        Chunk new_chunk = init_large_chunk(sizeof(char), false, string_len + 1);
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
    for (int i = 0; i < arena->string_num; i++)
        if (strncmp(strs[i], string, string_len) == 0 && strlen(strs[i]) == string_len)
            return strs[i];
   
    char *str = insert_string(string, string_len, arena);
    append_data(&str, 1, &arena->string_heads);
    arena->string_num++;

    return str;
}

MetaExpr *alloc_expr(Arena *arena)
{
    Chunk *last_chunk = LAST_DATA(&arena->exprs);

    if (!HAS_SPACE(last_chunk, 1))
    {
        Chunk new_chunk = init_chunk(sizeof(MetaExpr), false);
        append_data(&new_chunk, 1, &arena->exprs);
        last_chunk = LAST_DATA(&arena->exprs);
    }

    MetaExpr *head = alloc_mem(1, last_chunk);

    return head;
}

MetaExpr **alloc_exprs(int size, Arena *arena)
{
    Chunk *last_chunk = LAST_DATA(&arena->expr_lists);

    if (!HAS_SPACE(last_chunk, size))
    {
        Chunk new_chunk = init_large_chunk(sizeof(MetaExpr*), false, size);
        append_data(&new_chunk, 1, &arena->expr_lists);
        last_chunk = LAST_DATA(&arena->expr_lists);
    }

    MetaExpr **head = alloc_mem(size, last_chunk);

    return head;

}
