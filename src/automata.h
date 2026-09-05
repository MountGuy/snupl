#include "common.h"

#define NFA_TRANS(s, e, i, n) (n->trans[((s) * (n->state_num)  + (e)) * (n->char_num) + (i)])

void regist_char(char left, char right, NFA *nfa);
void gather_chars(GExpr *expr, NFA *nfa);
int find_char(char left, char right, NFA *nfa);

int alloc_NFA_state(NFA *nfa);
int count_state(GExpr *expr);
int count_char(GExpr *expr);
void build_NFA(GExpr *expr, NFA *nfa);
int _build_NFA(GExpr *expr, int start, NFA *nfa);

void find_reachable(NFA *nfa);
void absurb_eps(NFA *nfa);
int run_NFA(char *string, NFA *nfa);

void print_reachable(NFA *nfa);
void print_NFA(NFA *nfa);
