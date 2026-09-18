#ifndef ARENA_H
#define ARENA_H 1

#include "common.h"
#include "struct.h"
#include "chunk.h"

Arena init_arena();
char *insert_string(char *string, int string_len, Arena *arena);
void insert_string_head(char *string_head, Arena *arena);
char *add_string(char *string, int string_len, Arena *arena);
MetaExpr *alloc_expr(Arena *arena);
MetaExpr **alloc_exprs(int size, Arena *arena);

#endif
