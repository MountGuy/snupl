#include "character.h"
#include "arena.h"


void init_arena(Arena *arena)
{
    arena->buffer = (char*) malloc(sizeof(char) * DEF_SIZE);
    arena->strings = (char**) malloc(sizeof(char*) * DEF_SIZE);
    arena->exprs = (MetaExpr*) malloc(sizeof(MetaExpr) * DEF_SIZE);

    arena->buffer_used = 0;
    arena->string_used = 0;
    arena->expr_used = 0;
    arena->buffer_max = DEF_SIZE;
    arena->string_max = DEF_SIZE;
    arena->expr_max = DEF_SIZE;

}

char *add_string(char *string, int string_len, Arena *arena)
{
    for (int i = 0; i < arena->string_used; i++)
        if (strncmp(arena->strings[i], string, string_len) == 0 && strlen(arena->strings[i]) == string_len)
            return arena->strings[i];

    if (arena->string_max < arena->string_used + 10)
    {
        char **tmp = arena->strings;
        arena->strings = (char**) malloc(sizeof(char*) * arena->string_max * 2);
        memcpy(arena->strings, tmp, sizeof(char*) * arena->string_max);
        arena->string_max *= 2;
        free(tmp);
    }

    if (arena->buffer_max < arena->buffer_used + string_len + 10)
    {
        char *tmp = arena->buffer;
        arena->buffer = (char*) malloc(sizeof(char) * arena->buffer_max * 2);
        memcpy(arena->buffer, tmp, sizeof(char) * arena->buffer_max);

        for (int i = 0; i < arena->string_used; i++)
            arena->strings[i] = arena->strings[i] + (arena->buffer - tmp);

        arena->buffer_max *= 2;
        free(tmp);
    }

    memcpy(arena->buffer + arena->buffer_used, string, sizeof(char) * string_len);
    *(arena->buffer + arena->buffer_used + string_len) = c_null;
    arena->strings[arena->string_used] = arena->buffer + arena->buffer_used;

    char *return_val = arena->buffer + arena->buffer_used;
    arena->buffer_used += string_len + 1;
    arena->string_used++;

    return return_val;
}

MetaExpr *alloc_expr(Arena *arena)
{
    if (arena->expr_used == arena->expr_max)
    {
        MetaExpr *tmp = arena->exprs;
        arena->exprs = (MetaExpr*) malloc(sizeof(MetaExpr) * arena->expr_max * 2);
        memcpy(arena->exprs, tmp, sizeof(MetaExpr) * arena->expr_max);
        free(tmp);
    }
    return arena->exprs + arena->expr_used++;
}

