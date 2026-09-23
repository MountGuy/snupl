#ifndef PARSER_H
#define PARSER_H 1

#include "common.h"
#include "chunk.h"

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
    int token_num;

    int cursor;
} Parser;

FirstFollow solve_ff(Grammar *grammar);

#endif
