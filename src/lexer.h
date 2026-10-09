#ifndef LEXER_H
#define LEXER_H 1

#define I_EPS 0

#include "common.h"
#include "arena.h"

Chunk nfa_lexing(const char *input, Grammar *grammar, Arena *arena);
Chunk dfa_lexing(const char *input, Grammar *grammar, Arena *arena);

#endif
