#ifndef PARSER_H
#define PARSER_H 1

#include "common.h"
#include "chunk.h"
#include "arena.h"

typedef struct {
    ulli *sets;
    int set_num, set_size, offset, exact_set_size;
} FirstFollow;

typedef struct {
    FirstFollow *ff;
    int *can_eps;
    Chunk sub_sets, sup_sets;
} SetEqu;

typedef struct {
    Token *tokens;
    int token_num, cursor;
    FirstFollow *ff;
    Arena *arena;
} Parser;

Expr *parse(Chunk *chunk, Grammar *grammar, Arena *arena);

#endif
