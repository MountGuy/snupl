#ifndef LEXER_H
#define LEXER_H 1

#define I_EPS 0

#include "common.h"
#include "arena.h"

Chunk lexing(char *input, Grammar *grammar, Arena *arena);

#endif
