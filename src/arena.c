#include "arena.h"


void init_arena(Arena *arena)
{
    Chunk chunk;
    arena->strings = init_chunk(sizeof(Chunk), true);
    chunk = init_chunk(sizeof(char), false);
    append_data(&chunk, 1, &arena->strings);

    arena->string_heads = init_chunk(sizeof(void*), true);
    arena->string_num = 0;

    arena->exprs = init_chunk(sizeof(Chunk), true);
    chunk = init_chunk(sizeof(MetaExpr), false);
    append_data(&chunk, 1, &arena->exprs);

    arena->expr_lists = init_chunk(sizeof(Chunk), true);
    chunk = init_chunk(sizeof(MetaExpr*), false);
    append_data(&chunk, 1, &arena->expr_lists);
}

char *insert_string(char *string, int string_len, Arena *arena)
{
    Chunk last_buf;
    read_last(&last_buf, &arena->strings);

    if (!has_space(string_len + 1, &last_buf))
    {
        last_buf = init_chunk(sizeof(char), false);
        while (last_buf.max < string_len + 1)
            expand_chunk(&last_buf);
        append_data(&last_buf, 1, &arena->strings);
    }

    char *head = append_data(string, string_len, &last_buf);
    char null = c_null;
    append_data(&null, 1, &last_buf);
    write_last(&last_buf, &arena->strings);

    return head;
}

void insert_string_head(char *string_head, Arena *arena)
{
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
        last_buf = init_chunk(sizeof(MetaExpr), false);
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
        last_buf = init_chunk(sizeof(MetaExpr*), false);
        append_data(&last_buf, 1, &arena->expr_lists);
    }

    MetaExpr **head = alloc_mem(size, &last_buf);
    write_last(&last_buf, &arena->expr_lists);

    return head;

}
