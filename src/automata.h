#include "common.h"

void add_resource(char *string, NFA *nfa);
void gather_chars(GExpr *expr, NFA *nfa);

int alloc_NFA_state(NFA *nfa);
void add_trans(int start, int end, char c, NFA *nfa);
int can_trans(int start, int end, char c, NFA *nfa);

int count_state(GExpr *expr);
int build_NFA(GExpr *expr, NFA *nfa);
int _build_NFA(GExpr *expr, int start, NFA *nfa);
void find_reachable(NFA *nfa);

void print_reachable(NFA *nfa);
