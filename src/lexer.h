#ifndef LEXER_H
#define LEXER_H 1

#define I_EPS 0

#include "common.h"
#include "arena.h"

typedef struct {
    ulli *trans;
    int state_num, char_num, used_state_num;
    Chunk lubs;
    Grammar *grammar;
} NFABuilder;

typedef struct {
    ulli *trans;
    char *lbs, *ubs;
    TokenClass *tokcs;
    int state_num, char_num, exact_state_num;
    int *end_states, tokc_num;
} NFA;

typedef struct {
    int *lens, len;
    ulli *visiting, *tmp;
} NFAScanner;

Chunk lexing(char *input, Grammar *grammar, Arena *arena);

#endif
