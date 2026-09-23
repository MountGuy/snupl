#ifndef ARENA_H
#define ARENA_H 1

#include "common.h"
#include "chunk.h"

typedef struct {
    Chunk strings, string_heads, exprs, expr_lists;
    int string_num;
} Arena;

Arena init_arena();
char *add_string(char *string, int string_len, Arena *arena);
MetaExpr *alloc_expr(Arena *arena);
MetaExpr **alloc_exprs(int size, Arena *arena);

#endif
