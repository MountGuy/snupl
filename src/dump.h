#ifndef DUMP_H
#define DUMP_H 1

#include <stdio.h>

#include "struct.h"
#include "common.h"
#include "chunk.h"

void print_arena(Arena *arena);
void print_meta_token(MetaToken token);
void print_meta_lexer(MetaToken *tokens, int token_num);
void print_grammar(Grammar *grammar);
void print_meta_def(MetaDef *def);
void print_meta_expr(MetaExpr *expr);
void print_arena(Arena *arena);

void print_nfa(NFA *nfa, int debug);
void print_trans(int trim_state_num, int state_num, int char_num, ulli *trans);
void print_lexing_result(Chunk *tok_chunk);
void print_binary_vector(ulli *vector, int length);

#endif
