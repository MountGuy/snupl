#ifndef NFA_H
#define NFA_H 1

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

NFA build_NFA(Grammar *grammar);
void del_NFA(NFA *nfa);
void init_scanner(NFA *nfa, NFAScanner *scanner);
int step_NFA(uchar letter, NFA *nfa, NFAScanner *scanner);

#endif
