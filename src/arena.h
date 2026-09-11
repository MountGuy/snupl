#ifndef ARENA_H
#define ARENA_H 1

#include "common.h"
#include "struct.h"

#define DEF_SIZE 100

Chunk init_chunk(size_t unit, int length);
void expand_chunk(Chunk *chunk);
void *append_data(void *source, int length, Chunk *chunk);
void *alloc_mem(int length, Chunk *chunk);
void write_data(void *source, int idx, Chunk *chunk);
void write_last(void *source, Chunk *chunk);
void read_data(void *dest, int idx, Chunk *chunk);
void read_last(void *dest, Chunk *chunk);
int has_space(int length, Chunk *chunk);
void print_arena(Arena *arena);
void init_arena(Arena *arena);
char *insert_string(char *string, int string_len, Arena *arena);
void insert_string_head(char *string_head, Arena *arena);
char *add_string(char *string, int string_len, Arena *arena);
MetaExpr *alloc_expr(Arena *arena);
MetaExpr **alloc_exprs(int size, Arena *arena);

#endif
