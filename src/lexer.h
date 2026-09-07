#include "common.h"

#define NFA_TRANS(s, e, i, n) (n->trans[((s) * (n->state_num)  + (e)) * (n->char_num) + (i)])

int find_char(char left, char right, Lexer *lexer);
void build_NFA(GExpr *expr, NFA *nfa, Lexer *lexer);
int _build_NFA(GExpr *expr, int start, NFA *nfa, Lexer *lexer);
int count_state(GExpr *expr);
int alloc_NFA_state(NFA *nfa);
void find_reachable(NFA *nfa);
void absurb_eps(NFA *nfa);
void regist_char(char left, char right, Lexer *lexer);
void regist_string(char *string, Lexer *lexer);
void regist_assets(Asset *asset, Lexer *lexer);
void init_NFA_run(NFA *nfa);
int step_NFA(int *char_valid, NFA *nfa);
void lexing(GParser *parser, Lexer *lexer);
