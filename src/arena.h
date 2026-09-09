#ifndef ARENA_H
#define ARENA_H 1

#include <string.h>

#include "common.h"
#include "struct.h"

#define DEF_SIZE 100

void init_arena(int N, Arena *arena);
char *add_string(char *string, int string_len, Arena *arena);
MetaExpr *alloc_expr(Arena *arena);
MetaExpr **alloc_exprs(int size, Arena *arena);

#endif
