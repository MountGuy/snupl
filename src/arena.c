#include "character.h"
#include "arena.h"
#include "dump.h"


void init_arena(int N, Arena *arena)
{
    arena->buffer = (char*) malloc(sizeof(char) * N);
    arena->strings = (char**) malloc(sizeof(char*) * N);
    arena->exprs = (MetaExpr*) malloc(sizeof(MetaExpr) * N);
    arena->expr_lists = (MetaExpr**) malloc(sizeof(MetaExpr*) * N);

    arena->buffer_used = 0;
    arena->string_used = 0;
    arena->expr_used = 0;
    arena->expr_list_used = 0;
    arena->buffer_max = N;
    arena->string_max = N;
    arena->expr_max = N;
    arena->expr_list_max = N;

}

char *add_string(char *string, int string_len, Arena *arena)
{
    for (int i = 0; i < arena->string_used; i++)
        if (strncmp(arena->strings[i], string, string_len) == 0 && strlen(arena->strings[i]) == string_len)
            return arena->strings[i];

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
    return arena->exprs + arena->expr_used++;
}

MetaExpr **alloc_exprs(int size, Arena *arena)
{
    MetaExpr **return_val = arena->expr_lists + arena->expr_list_used;
    arena->expr_list_used += size;
    return return_val;
}

