#ifndef LEXER_H
#define LEXER_H 1

#define I_EPS 0

#include "common.h"
#include "arena.h"
#include "dfa.h"
#include "nfa.h"

const char *nfa_scan(const char *input, NFA *nfa);
Chunk lexing_through_nfa(const char *input, Grammar *grammar, Arena *arena);
const char *dfa_scan(const char *input, DFA *dfa);
Chunk lexing_through_dfa(const char *input, Grammar *grammar, Arena *arena);


#endif
