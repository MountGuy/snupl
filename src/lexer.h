#ifndef LEXER_H
#define LEXER_H 1

#define C_EPS ('\0')
#define I_EPS 0

#include "common.h"
#include "arena.h"
#include "bitop.h"

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

int alloc_NFA_state(NFABuilder *builder);
void add_char(char lb, char ub, Chunk *lubs);
int find_char(char lb, char ub, Chunk *lubs);

int count_state(MetaExpr *expr, Grammar *grammar);
void gather_char(MetaExpr *expr, Chunk *lubs);

int can_trans(int start, int cdx, int end, NFABuilder *builder);
void add_trans(int start, int cdx, int end, NFABuilder *builder);

int _build_NFA(MetaExpr *expr, int start, NFABuilder *builder);
void postproc_trans(NFABuilder *builder);
NFA build_NFA(Grammar *grammar);

void init_scanner(NFA *nfa, NFAScanner *scanner);
int step_NFA(char letter, NFA *nfa, NFAScanner *scanner);
Chunk lexing(char *input, Grammar *grammar, Arena *arena);

#endif
