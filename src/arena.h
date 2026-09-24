#ifndef ARENA_H
#define ARENA_H 1

#include "common.h"
#include "chunk.h"

typedef struct {
    Chunk strings, string_heads, string_lens, mexprs, exprs;
    int string_num;
} Arena;

Arena init_arena();
char *add_string(char *string, int string_len, Arena *arena);
MetaExpr *alloc_mexpr(int size, Arena *arena);
Expr *alloc_expr(int size, Arena *arena);

#endif
