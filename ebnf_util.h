#ifndef EBNF_UTIL
#define EBNF_UTIL 1
#include "common.h"
#include "ebnf.h"

void advance_parser(Parser *parser);
Token *peek_tok(Parser *parser);
Expr *alloc_arena(Parser *parser);
int parser_end(Parser *parser);
void print_expr(Expr *expr);
void print_parser(Parser *parser);

#endif