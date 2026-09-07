#include "common.h"

#define IS_LEXING_TERM(s) ((s)[0] == '_' && (s)[1] != '_')
#define SKIP_SPACE(p) while (*(p) == ' ' || *(p) == '\t' || *(p) == '\n') (p)++

int _build_NFA(GExpr *expr, int start, NFA *nfa, Lexer *lexer);
void build_NFA(GExpr *expr, NFA *nfa, Lexer *lexer);

void init_NFA_run(NFA *nfa);
int step_NFA(int *char_valid, NFA *nfa);
void lexing(GParser *parser, Lexer *lexer);
