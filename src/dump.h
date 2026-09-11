#ifndef DUMP_H
#define DUMP_H 1

#include "struct.h"
#include "common.h"

void print_meta_token(MetaToken token);
void print_meta_lexer(MetaLexer *lexer);
void print_grammar(Grammar *grammar);
void print_meta_def(MetaDef *def);
void print_meta_expr(MetaExpr *expr);
void print_arena(Arena *arena);

void print_nfa(NFA *nfa);
void print_trans(int state_num, int char_num, char ***trans);
void print_lexing_result(Lexer *lexer);

#endif
