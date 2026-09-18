#ifndef DUMP_H
#define DUMP_H 1

#include <stdio.h>

#include "struct.h"
#include "common.h"
#include "chunk.h"

void print_meta_token(MetaToken token);
void print_meta_lexer(MetaLexer *lexer);
void print_grammar(Grammar *grammar);
void print_meta_def(MetaDef *def);
void print_meta_expr(MetaExpr *expr);
void print_arena(Arena *arena);

void print_nfa(NFA *nfa, int debug);
void print_trans(int trim_state_num, int state_num, int char_num, unsigned long long int *trans);
void print_lexing_result(Lexer *lexer);
void print_binary_vector(unsigned long long int *vector, int length);

#endif
