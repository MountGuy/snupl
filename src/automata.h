#include "common.h"

#define NFA_TRANS(s, e, i, n) (n->trans[((s) * (n->state_num)  + (e)) * (n->char_num) + (i)])

void regist_char(char left, char right, NFA *nfa);
void gather_chars(GExpr *expr, NFA *nfa);

int alloc_NFA_state(NFA *nfa);
void add_trans(int start, int end, int c, NFA *nfa);
int can_trans(int start, int end, int c, NFA *nfa);

int count_state(GExpr *expr);
int build_NFA(GExpr *expr, NFA *nfa);
int _build_NFA(GExpr *expr, int start, NFA *nfa);
void find_reachable(NFA *nfa);
void absurb_eps(NFA *nfa);
void print_NFA(NFA *nfa);

void print_reachable(NFA *nfa);
int run_NFA(char *string, NFA *nfa);
