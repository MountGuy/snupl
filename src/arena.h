#ifndef ARENA_H
#define ARENA_H 1

#include <string.h>

#include "common.h"
#include "struct.h"

#define DEF_SIZE 100

void init_arena(Arena *arena);
char *add_string(char *string, int string_len, Arena *arena);
MetaExpr *alloc_expr(Arena *arena);

#endif
