#ifndef PARSER_H
#define PARSER_H 1

#include "common.h"
#include "chunk.h"
#include "arena.h"

typedef struct {
    ulli *sets, *old_sets;
    int set_num, set_size, mem_size, offset, exact_set_size;
} FirstFollow;

typedef struct {
    int *can_eps;
    Chunk sub_idx, sup_idx;
} SetEqu;

typedef struct {
    Token *tokens;
    int token_num, cursor;
    FirstFollow *ff;
    Arena *arena;
} Parser;

Expr *parse(Chunk *chunk, Grammar *grammar, Arena *arena);

#endif
