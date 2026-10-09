#ifndef DUMP_H
#define DUMP_H 1

#include <stdio.h>

#include "common.h"
#include "chunk.h"
#include "arena.h"
#include "meta.h"
#include "nfa.h"
#include "dfa.h"
#include "lexer.h"
#include "parser.h"

char *utf8_to_str(ulli code, char *buffer);
void print_arena(Arena *arena);
void print_meta_token(MetaToken token);
void print_meta_lexer(Chunk *tok_chunk);
void print_grammar(Grammar grammar);
void print_meta_def(MetaDef *def);
void print_meta_expr(MetaExpr *expr);
void print_arena(Arena *arena);

void print_nfa(NFA *nfa, int debug);
void print_dfa(DFA dfa, int debug);
void print_trans(int trim_state_num, int state_num, int char_num, ulli *trans);
void print_lexing_result(Chunk *tok_chunk);
void print_binary_vector(ulli *vector, int length);
void print_ff_bin(FirstFollow *ff);
void print_ff(FirstFollow *ff, Grammar *grammar);
void print_ff_tp(FirstFollow *ff, Grammar *grammar);
void print_token(Token token);
void print_expr(Expr *expr, int depth);

#endif
