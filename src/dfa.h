#ifndef DFA_H
#define DFA_H 1

#include "common.h"
#include "chunk.h"
#include "nfa.h"

typedef struct {
    int used_state_num, state_size;
    Chunk trans;
    Chunk states;
    NFA *nfa;
} DFABuilder;

typedef struct {
    int state_num, (*trans)[256];
    int *tokc_output, tokc_num;
    TokenClass *tokcs;
} DFA;

DFA build_DFA(Grammar *grammar);
void del_DFA(DFA *dfa);

#endif
