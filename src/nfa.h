#ifndef AUTOMATA_H
#define AUTOMATA_H 1

#define I_EPS 0

#include "common.h"
#include "chunk.h"

typedef struct {
    ulli *trans;
    int state_num, char_num, used_state_num;
    Chunk lubs;
    Grammar *grammar;
} NFABuilder;

typedef struct {
    ulli *trans;
    uchar *lbs, *ubs;
    TokenClass *tokcs;
    int state_num, char_num, exact_state_num;
    int *end_states, tokc_num;
} NFA;

typedef struct {
    int *lens, len;
    ulli *visiting, *tmp;
} NFAScanner;

typedef struct {
    int *trans, used_state_num, state_byte_size;
    ulli *chars;
    int char_num;
    Chunk states;
    NFA *nfa;
} DFABuilder;

typedef struct {
    int *trans;
    ulli *lbs, *ubs;
    TokenClass *tokcs;
    int state_num, char_num;
    int tokc_num, *end_tokc;
} DFA;

NFA build_NFA(Grammar *grammar);
void del_NFA(NFA *nfa);
void init_scanner(NFA *nfa, NFAScanner *scanner);
int step_NFA(uchar letter, int width, NFA *nfa, NFAScanner *scanner);

#endif
