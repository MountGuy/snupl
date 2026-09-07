#include "common.h"

#define NFA_TRANS(s, e, i, n) (n->trans[((s) * (n->state_num)  + (e)) * (n->char_num) + (i)])

int find_char(char lb, char ub, Lexer *lexer);
int count_state(GExpr *expr);
int alloc_NFA_state(NFA *nfa);
void find_reachable(NFA *nfa);
void absurb_eps(NFA *nfa);

void _regist_char(char lb, char ub, Lexer *lexer);
void regist_char(Asset *asset, Lexer *lexer);
